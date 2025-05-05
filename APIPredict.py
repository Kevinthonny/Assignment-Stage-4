from flask import Flask, request, jsonify
import torch
import requests
from torch import nn
from torch import optim

# Inisialisasi Flask
app = Flask(__name__)

# Muat Model yang Telah Dilatih
model = ServoNet()
model.load_state_dict(torch.load('solar_tracker_model.pth'))
model.eval()

# Ubidots API Configuration
UBIDOTS_API_KEY = 'BBUS-texNJUy7hZEzqgMHrcKGHAkbGE0fxI'  # Ganti dengan token API Anda
UBIDOTS_DEVICE_LABEL = 'esp-32'  # Ganti dengan label perangkat Anda di Ubidots
UBIDOTS_URL = f'https://industrial.api.ubidots.com/api/v1.6/devices/{UBIDOTS_DEVICE_LABEL}/'

# Neural Network Model
class ServoNet(nn.Module):
    def __init__(self):
        super(ServoNet, self).__init__()
        self.fc1 = nn.Linear(6, 32)
        self.fc2 = nn.Linear(32, 16)
        self.fc3 = nn.Linear(16, 2)
        
    def forward(self, x):
        x = torch.relu(self.fc1(x))
        x = torch.relu(self.fc2(x))
        x = self.fc3(x)
        return x

# Fungsi untuk Prediksi Servo
def predict_servo_movement(ldr_values, current):
    # Normalisasi Input
    ldr_values_normalized = [ldr / 4095.0 for ldr in ldr_values]
    input_tensor = torch.tensor([ldr_values_normalized + [current]], dtype=torch.float32)
    with torch.no_grad():
        prediction = model(input_tensor).numpy()
    return prediction[0]

@app.route('/predict', methods=['POST'])
def predict():
    # Terima data sensor dari ESP32
    data = request.get_json()
    ldr_values = [data['ldr1'], data['ldr2'], data['ldr3'], data['ldr4']]
    current = data['current']
    
    # Prediksi pergerakan servo
    predicted_angles = predict_servo_movement(ldr_values, current)
    
    # Kirim data ke Ubidots
    payload = {
        "servo_x": predicted_angles[0],
        "servo_y": predicted_angles[1],
        "current": current
    }

    headers = {
        "Content-Type": "application/json",
        "X-Auth-Token": UBIDOTS_API_KEY
    }

    response = requests.post(UBIDOTS_URL, json=payload, headers=headers)

    if response.status_code == 201:
        return jsonify({"message": "Data successfully sent to Ubidots", "predicted_angles": predicted_angles}), 200
    else:
        return jsonify({"message": "Failed to send data to Ubidots", "error": response.text}), 500

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
