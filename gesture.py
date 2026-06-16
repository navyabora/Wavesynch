import sys #import python's built in libraries into the script + this helps in handling errors and program terminiation
import time #to calculate the time or delay/hold time
import serial 
import serial.tools.list_ports

try:
    from pynput.keyboard import Key, Controller
except ImportError:
    sys.exit("Missing dependency — run:  pip install pynput pyserial")



BAUD_RATE = 115200 #bits per second. the no. of bits that the arduino is sending to the computer via serial communication


COM_PORT = "COM3"


CMD_MAP = {
    "VOL_UP"    : Key.media_volume_up,
    "VOL_DOWN"  : Key.media_volume_down,
    "PLAY_PAUSE": Key.media_play_pause,
    "FORWARD"   : Key.right,   
    "REWIND"    : Key.left,    
}


LABELS = {
    "VOL_UP"      : "Volume Up",
    "VOL_DOWN"    : "Volume Down",
    "PLAY_PAUSE"  : "Play / Pause",
    "FORWARD"     : "Forward (seek)",
    "REWIND"      : "Rewind  (seek)",
    "LEFT_LOCK"   : " Left hand locked   [Volume mode]",
    "RIGHT_LOCK"  : "Right hand locked  [Seek mode]",
    "LEFT_UNLOCK" : "Left hand released",
    "RIGHT_UNLOCK": "Right hand released",
    "READY"       : "Arduino ready",
}


def find_arduino():
    keywords = ("arduino", "ch340", "ch341", "usb serial", "cp210")
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if any(k in (p.description or "").lower() for k in keywords):
            return p.device
    return ports[0].device if ports else None



def main():
    global COM_PORT

    if COM_PORT is None:
        COM_PORT = find_arduino()
        if COM_PORT is None:
            sys.exit("No serial port found. Set COM_PORT manually at the top of this file.")
        print(f"  Auto-detected port: {COM_PORT}")

    kb = Controller()

    print(f"  Connecting to {COM_PORT} @ {BAUD_RATE} baud …")
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
    except serial.SerialException as e:
        sys.exit(f"Could not open port: {e}")

    
    time.sleep(2.0)
    print("  Running. Press Ctrl+C to quit.\n")
    print("  " + "─" * 38)

    while True:
        try:
            raw = ser.readline()
            if not raw:
                continue

            line = raw.decode("utf-8", errors="ignore").strip()

            
            if not line or line.startswith("DBG"):
                continue

            if not line.startswith("CMD:"):
                continue

            cmd = line[4:]   

            
            if cmd in CMD_MAP:
                key = CMD_MAP[cmd]
                kb.press(key)
                kb.release(key)

            
            print("  " + LABELS.get(cmd, f"[{cmd}]"))

        except KeyboardInterrupt:
            print("\n\n  Exiting …")
            ser.close()
            break

        except UnicodeDecodeError:
            continue

        except serial.SerialException as e:
            print(f"\n  Serial error: {e}")
            print("  Attempting reconnect in 3 s …")
            time.sleep(3)
            try:
                ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
                print("  Reconnected.\n")
            except serial.SerialException:
                print("  Reconnect failed. Check the cable.\n")

        except Exception as e:
            print(f"  Unexpected error: {e}")
            time.sleep(0.5)


if __name__ == "__main__":
    main()
