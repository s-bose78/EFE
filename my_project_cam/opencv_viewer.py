import requests
import cv2
import numpy as np
import time  # For controlling frame fetch rate

stream_url = "http://192.168.1.92/capture"  # Replace with the correct URL

while True:
    try:
        response = requests.get(stream_url, stream=True)

        if response.status_code == 200:
            # Convert the image bytes into a numpy array
            img_array = np.array(bytearray(response.content), dtype=np.uint8)
            
            # Decode the JPEG image into an OpenCV image
            frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
            
            if frame is not None:
                cv2.imshow("ESP32 Stream", frame)

            # Add a delay here to control the frame fetch rate (for example, 10 FPS)
            time.sleep(0.01)  # 100ms delay for ~10 FPS

        else:
            print(f"Failed to fetch image. HTTP Status Code: {response.status_code}")
            break

    except requests.exceptions.RequestException as e:
        print("Error while making request:", e)
        break
    except Exception as e:
        print("General error:", e)
        break

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()

