import os, json, secrets
from datetime import datetime, timezone
from typing import Literal
from fastapi import FastAPI, Header, HTTPException, Query, Response
from pydantic import BaseModel, Field
from pymongo import MongoClient
from dotenv import load_dotenv
import paho.mqtt.publish as publish

load_dotenv()
app = FastAPI()
db = MongoClient(os.getenv("MONGODB_URI"))["test2"]
fans = db["fans"]
readings = db["readings"]

class FanIn(BaseModel):
    mac: str
    room: str

@app.post("/fans", status_code=201)
def create_fan(fan: FanIn):
    if fans.find_one({"mac": fan.mac}):
        raise HTTPException(status_code=409, detail="Fan already exists")
    key = secrets.token_urlsafe(16)
    fans.insert_one({"mac": fan.mac, "room": fan.room, "key": key})
    return {"mac": fan.mac, "room": fan.room, "key": key}

@app.get("/fans")
def list_fans():
    return list(fans.find({}, {"_id": 0, "key": 0}))

@app.get("/fans/{mac}")
def get_fan(mac: str):
    fan = fans.find_one({"mac": mac}, {"_id": 0, "key": 0})
    if not fan:
        raise HTTPException(status_code=404, detail="Fan not found")
    return fan

class ReadingIn(BaseModel):
    mac: str
    humidity: float = Field(ge=0, le=100)

@app.post("/readings", status_code=201)
def create_reading(reading: ReadingIn, x_api_key: str = Header(default=None)):
    if not x_api_key:
        raise HTTPException(status_code=401, detail="Missing API key")
    owner = fans.find_one({"key": x_api_key})
    if not owner:
        raise HTTPException(status_code=401, detail="Unknown API key")
    if owner["mac"] != reading.mac:
        raise HTTPException(status_code=403, detail="Key belongs to another fan")
    now = datetime.now(timezone.utc)
    readings.insert_one({"mac": reading.mac, "humidity": reading.humidity, "time": now})
    return {"mac": reading.mac, "humidity": reading.humidity, "time": now.isoformat()}

@app.get("/readings")
def list_readings(mac: str = None, limit: int = Query(default=10, ge=1, le=100)):
    query = {"mac": mac} if mac else {}
    docs = readings.find(query, {"_id": 0}).sort("time", -1).limit(limit)
    return [{**d, "time": d["time"].isoformat()} for d in docs]