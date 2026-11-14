# ECSE3038 Test 2: The bathroom fan

An ESP32 (Wokwi) sends humidity readings to a FastAPI and MongoDB Atlas API. The API sends fan settings to the board over MQTT.

## Run the API
cd api, create a venv, run `pip install -r requirements.txt`, make a `.env` with MONGODB_URI and MQTT_NAME, then run `uvicorn app:app --host 127.0.0.1 --port 8000`.

## Run the board
Open the board folder in VS Code with PlatformIO, make include/secrets.h with the fan's key, build, then start the Wokwi simulator.