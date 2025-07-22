# Importamos librerias
import torch
import cv2
import numpy as np
import pandas

# leemos el modelo
model = torch.hub.load(
    'ultralytics/yolov5',
    'custom',
    path='C:/visionartificial/model/cafe.pt',
    force_reload=True
)
# realizamos videocaptura

cap = cv2.VideoCapture(0)


# empezamos con nuestro while true
while True:
    # Realizamos Video Captura
    ret, frame = cap.read()

    # realizamos deteccion
    detect = model(frame)
    info = detect.pandas().xyxy[0]
    print(info)

    # mostramos FPS

    cv2.imshow('detector de cafe', np.squeeze(detect.render()))

    t = cv2.waitKey(5)
    if t == 27:
        break

cap.release()
cv2.destroyAllWindows()
