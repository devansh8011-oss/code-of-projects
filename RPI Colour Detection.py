from flask import Flask, Response
from picamera2 import Picamera2
import cv2
import numpy as np
import time
import serial
from collections import deque

# ======================================
# UART
# ======================================

ser = serial.Serial('/dev/serial0', 9600)

app = Flask(__name__)

# ======================================
# CAMERA
# ======================================

picam2 = Picamera2()
picam2.configure(
    picam2.create_preview_configuration(
        main={"size": (640, 480)}
    )
)
picam2.start()
time.sleep(2)

picam2.set_controls({
    "AeEnable":     False,
    "AwbEnable":    False,
    "ExposureTime": 20000
})

# ======================================
# TUNE THESE — run calibration mode
# first, place bot on each tile and
# note the printed values, then adjust
# ======================================

THRESHOLDS = {

    "BLACK": {
        "v_max":   60,    # HSV: very dark
        "l_max":   50,    # LAB: low lightness
    },

    "WHITE": {
        "v_min":   180,   # HSV: very bright
        "s_max":   50,    # HSV: very low saturation
        "l_min":   170,   # LAB: high lightness
    },

    "SILVER": {
        "v_min":   80,    # HSV: mid brightness
        "v_max":   179,   # HSV: below white
        "s_max":   60,    # HSV: low saturation
        "l_min":   80,    # LAB: mid lightness
        "l_max":   169,   # LAB: below white
    },

    "BLUE": {
        "h_min":   100,   # HSV hue range (OpenCV 0-179)
        "h_max":   130,
        "s_min":   100,   # must be vivid — kills skin/grey bleed
        "v_min":   40,
        "b_min":   120,   # BGR: blue channel dominance
        "rg_max":  200,   # BGR: red+green must be lower than blue
    },
}

# how many frames color must hold before UART fires
DEBOUNCE_FRAMES = 5

# % of pixels that must agree with detected color
MIN_CONFIDENCE  = 60

# history window for temporal smoothing (frames)
HISTORY_LEN     = 7

# ======================================
# CLASSIFIER — 3 independent methods
# voted for final decision
# ======================================

def classify_hsv(h, s, v):
    """Primary classification on median HSV."""

    t = THRESHOLDS

    if v < t["BLACK"]["v_max"]:
        return "BLACK"

    if s < t["WHITE"]["s_max"] and v > t["WHITE"]["v_min"]:
        return "WHITE"

    if (s < t["SILVER"]["s_max"]
            and t["SILVER"]["v_min"] < v < t["SILVER"]["v_max"]):
        return "SILVER"

    if (t["BLUE"]["h_min"] < h < t["BLUE"]["h_max"]
            and s > t["BLUE"]["s_min"]
            and v > t["BLUE"]["v_min"]):
        return "BLUE"

    return "UNKNOWN"


def classify_lab(l, a, b_ch):
    """
    Secondary classification on LAB.
    LAB is perceptually uniform — much more
    stable under lighting changes than HSV.
    L = lightness, a = green-red, b = blue-yellow
    """

    t = THRESHOLDS

    if l < t["BLACK"]["l_max"]:
        return "BLACK"

    if l > t["WHITE"]["l_min"]:
        return "WHITE"

    # silver: mid L, near-neutral a and b
    if (t["SILVER"]["l_min"] < l < t["SILVER"]["l_max"]
            and 110 < a < 140   # near neutral (128 = neutral in OpenCV LAB)
            and 110 < b_ch < 145):
        return "SILVER"

    # blue: low b channel (b < 128 = blue in LAB)
    if b_ch < 115 and l > 20:
        return "BLUE"

    return "UNKNOWN"


def classify_bgr(b_med, g_med, r_med):
    """
    Tertiary classification using raw BGR
    channel ratios. Simple but lighting-independent
    when channels are compared relatively.
    """

    total = float(b_med + g_med + r_med) + 1e-6

    r_ratio = r_med / total
    g_ratio = g_med / total
    b_ratio = b_med / total

    brightness = (b_med + g_med + r_med) / 3.0

    if brightness < 55:
        return "BLACK"

    if brightness > 170 and abs(r_ratio - g_ratio) < 0.08:
        return "WHITE"

    # silver: high brightness, balanced channels
    if brightness > 90 and abs(r_ratio - b_ratio) < 0.07:
        return "SILVER"

    # blue: b channel dominates
    if b_ratio > 0.40 and b_med > r_med + 30 and b_med > g_med + 20:
        return "BLUE"

    return "UNKNOWN"


def vote(hsv_result, lab_result, bgr_result):
    """
    Majority vote across 3 classifiers.
    Any 2-of-3 agreement wins.
    Falls back to HSV if all 3 disagree.
    """

    votes = [hsv_result, lab_result, bgr_result]

    for candidate in ["BLACK", "WHITE", "SILVER", "BLUE"]:
        if votes.count(candidate) >= 2:
            return candidate

    # all 3 disagree — trust HSV as primary
    return hsv_result


# ======================================
# CONFIDENCE
# pixel-level mask agreement on full frame
# ======================================

def get_confidence(hsv_frame, lab_frame, color_name):

    h = hsv_frame[:, :, 0]
    s = hsv_frame[:, :, 1]
    v = hsv_frame[:, :, 2]

    L = lab_frame[:, :, 0]
    a = lab_frame[:, :, 1]
    b = lab_frame[:, :, 2]

    t = THRESHOLDS

    if color_name == "BLACK":
        mask = (v < t["BLACK"]["v_max"]) & (L < t["BLACK"]["l_max"] + 10)

    elif color_name == "WHITE":
        mask = (s < t["WHITE"]["s_max"] + 10) & (v > t["WHITE"]["v_min"] - 10)

    elif color_name == "SILVER":
        mask = (
            (s < t["SILVER"]["s_max"] + 10)
            & (v > t["SILVER"]["v_min"] - 10)
            & (v < t["SILVER"]["v_max"])
            & (L > t["SILVER"]["l_min"] - 10)
        )

    elif color_name == "BLUE":
        mask = (
            (h > t["BLUE"]["h_min"] - 5)
            & (h < t["BLUE"]["h_max"] + 5)
            & (s > t["BLUE"]["s_min"] - 15)
        )

    else:
        return 0.0

    total = hsv_frame.shape[0] * hsv_frame.shape[1]
    return round((np.sum(mask) / total) * 100, 1)


# ======================================
# DRAW COLORS
# ======================================

DRAW_COLORS = {
    "BLACK":   (50,  50,  50),
    "WHITE":   (255, 255, 255),
    "SILVER":  (180, 180, 180),
    "BLUE":    (255, 80,  0),
    "UNKNOWN": (0,   220, 0),
}

# ======================================
# STREAM
# ======================================

def generate_frames():

    last_sent    = ""
    stable_color = ""
    stable_count = 0
    frame_num    = 0

    # rolling history for temporal smoothing
    # vote across last N frames too
    history = deque(maxlen=HISTORY_LEN)

    while True:

        frame = picam2.capture_array()

        # median blur — kills hot pixels, glare
        frame = cv2.medianBlur(frame, 5)

        # ======================================
        # COMPUTE ALL 3 COLOR SPACES
        # ======================================

        hsv_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        lab_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2LAB)

        # full frame medians — entire tile surface
        med_h = int(np.median(hsv_frame[:, :, 0]))
        med_s = int(np.median(hsv_frame[:, :, 1]))
        med_v = int(np.median(hsv_frame[:, :, 2]))

        med_l = int(np.median(lab_frame[:, :, 0]))
        med_a = int(np.median(lab_frame[:, :, 1]))
        med_b = int(np.median(lab_frame[:, :, 2]))

        med_bgr_b = int(np.median(frame[:, :, 0]))
        med_bgr_g = int(np.median(frame[:, :, 1]))
        med_bgr_r = int(np.median(frame[:, :, 2]))

        # ======================================
        # 3-WAY VOTE
        # ======================================

        hsv_vote = classify_hsv(med_h, med_s, med_v)
        lab_vote = classify_lab(med_l, med_a, med_b)
        bgr_vote = classify_bgr(med_bgr_b, med_bgr_g, med_bgr_r)

        frame_color = vote(hsv_vote, lab_vote, bgr_vote)

        # ======================================
        # TEMPORAL SMOOTHING
        # majority vote across last N frames
        # eliminates single-frame glitches
        # ======================================

        history.append(frame_color)

        color_counts = {}
        for c in history:
            color_counts[c] = color_counts.get(c, 0) + 1

        # pick most frequent color in history window
        color_name = max(color_counts, key=color_counts.get)

        confidence = get_confidence(hsv_frame, lab_frame, color_name)

        draw_color = DRAW_COLORS.get(color_name, (0, 220, 0))

        # ======================================
        # DEBOUNCE + CONFIDENCE GATE
        # ======================================

        if color_name == stable_color:
            stable_count += 1
        else:
            stable_color = color_name
            stable_count = 1

        ready = (
            stable_count >= DEBOUNCE_FRAMES
            and color_name != last_sent
            and confidence >= MIN_CONFIDENCE
            and color_name != "UNKNOWN"
        )

        if ready:
            ser.write((color_name + "\n").encode())
            print(
                f"[SEND] {color_name:7s} | "
                f"HSV:{med_h},{med_s},{med_v} | "
                f"LAB:{med_l},{med_a},{med_b} | "
                f"BGR:{med_bgr_r},{med_bgr_g},{med_bgr_b} | "
                f"votes: HSV={hsv_vote} LAB={lab_vote} BGR={bgr_vote} | "
                f"conf:{confidence}%"
            )
            last_sent = color_name

        # calibration log every 60 frames
        frame_num += 1
        if frame_num % 60 == 0:
            print(
                f"[CAL]  {color_name:7s} | "
                f"HSV:{med_h:3d},{med_s:3d},{med_v:3d} | "
                f"LAB:{med_l:3d},{med_a:3d},{med_b:3d} | "
                f"votes HSV={hsv_vote} LAB={lab_vote} BGR={bgr_vote} | "
                f"conf:{confidence}% stable:{stable_count}"
            )

        # ======================================
        # OVERLAY
        # ======================================

        # full border shows current detection
        cv2.rectangle(
            frame, (0, 0),
            (frame.shape[1]-1, frame.shape[0]-1),
            draw_color, 30
        )

        # color name + confidence — shadow then color
        label = f"{color_name}  {confidence}%"
        for offset, col, thick in [((2,2),(0,0,0),5), ((0,0),draw_color,3)]:
            cv2.putText(
                frame, label,
                (20 + offset[0], 56 + offset[1]),
                cv2.FONT_HERSHEY_SIMPLEX, 1.4, col, thick
            )

        # 3 classifier votes shown individually
        vote_text = f"HSV:{hsv_vote[:3]}  LAB:{lab_vote[:3]}  BGR:{bgr_vote[:3]}"
        for offset, col, thick in [((1,1),(0,0,0),3), ((0,0),(180,180,180),1)]:
            cv2.putText(
                frame, vote_text,
                (20 + offset[0], 95 + offset[1]),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, col, thick
            )

        # raw values
        raw_text = f"H:{med_h} S:{med_s} V:{med_v}  L:{med_l} a:{med_a} b:{med_b}"
        for offset, col, thick in [((1,1),(0,0,0),3), ((0,0),(160,160,160),1)]:
            cv2.putText(
                frame, raw_text,
                (20 + offset[0], 120 + offset[1]),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, col, thick
            )

        # stability bar
        bx, by, bw, bh = 20, 132, 200, 10
        fill = int((min(stable_count, DEBOUNCE_FRAMES) / DEBOUNCE_FRAMES) * bw)
        cv2.rectangle(frame, (bx, by), (bx+bw, by+bh), (30,30,30), -1)
        cv2.rectangle(frame, (bx, by), (bx+fill, by+bh), draw_color, -1)

        # temporal history bar
        # shows last N frame colors as colored dots
        dot_r = 7
        for i, past_color in enumerate(history):
            dot_col = DRAW_COLORS.get(past_color, (0,220,0))
            cx_dot  = 20 + i * (dot_r * 2 + 4)
            cy_dot  = frame.shape[0] - 30
            cv2.circle(frame, (cx_dot, cy_dot), dot_r, dot_col, -1)
            cv2.circle(frame, (cx_dot, cy_dot), dot_r, (80,80,80), 1)

        # sent label bottom right
        sent_col = (0,200,0) if last_sent == color_name else (80,80,80)
        sent_txt = f"SENT: {last_sent if last_sent else '---'}"
        cv2.putText(
            frame, sent_txt,
            (frame.shape[1] - 220, frame.shape[0] - 16),
            cv2.FONT_HERSHEY_SIMPLEX, 0.6, sent_col, 2
        )

        # ======================================
        # ENCODE
        # ======================================

        _, buffer = cv2.imencode('.jpg', frame)
        frame = buffer.tobytes()

        yield (
            b'--frame\r\n'
            b'Content-Type: image/jpeg\r\n\r\n' +
            frame + b'\r\n'
        )

# ======================================
# ROUTE + MAIN
# ======================================

@app.route('/')
def video_feed():
    return Response(
        generate_frames(),
        mimetype='multipart/x-mixed-replace; boundary=frame'
    )

if __name__ == '__main__':
    print("=" * 50)
    print("TILE COLOR DETECTOR — 3-space voting engine")
    print("Place bot on each tile, watch [CAL] to tune")
    print("=" * 50)
    app.run(host='0.0.0.0', port=5000)
