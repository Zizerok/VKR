import json
import os
import random
import sqlite3
from pathlib import Path
from typing import Any

import requests
from dotenv import load_dotenv
from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel


BASE_DIR = Path(__file__).resolve().parent
load_dotenv(BASE_DIR / ".env")

VK_GROUP_ID = os.getenv("VK_GROUP_ID", "")
VK_COMMUNITY_TOKEN = os.getenv("VK_COMMUNITY_TOKEN", "")
VK_CONFIRMATION_CODE = os.getenv("VK_CONFIRMATION_CODE", "")
VK_SECRET_KEY = os.getenv("VK_SECRET_KEY", "")
VK_API_VERSION = os.getenv("VK_API_VERSION", "5.199")
DB_PATH = BASE_DIR / os.getenv("BACKEND_DB_PATH", "backend.db")

app = FastAPI(title="VKR VK Bot Backend")


class ShiftPosition(BaseModel):
    position: str
    count: int = 1


class ShiftNotificationRequest(BaseModel):
    shift_id: int
    message: str
    user_ids: list[int]
    open_positions: list[ShiftPosition] = []


def db_connection() -> sqlite3.Connection:
    connection = sqlite3.connect(DB_PATH)
    connection.row_factory = sqlite3.Row
    return connection


def init_db() -> None:
    with db_connection() as connection:
        connection.execute(
            """
            CREATE TABLE IF NOT EXISTS shift_responses (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                shift_id INTEGER NOT NULL,
                vk_id TEXT NOT NULL,
                position_name TEXT,
                response_status TEXT NOT NULL,
                response_message TEXT,
                raw_payload TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                processed_at TEXT
            )
            """
        )
        connection.execute(
            """
            CREATE TABLE IF NOT EXISTS sent_notifications (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                shift_id INTEGER,
                user_id INTEGER,
                message_text TEXT,
                send_status TEXT NOT NULL,
                error_text TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
            """
        )


@app.on_event("startup")
def on_startup() -> None:
    init_db()


def vk_api(method: str, params: dict[str, Any]) -> dict[str, Any]:
    if not VK_COMMUNITY_TOKEN:
        raise HTTPException(status_code=500, detail="VK_COMMUNITY_TOKEN is not configured")

    response = requests.post(
        f"https://api.vk.com/method/{method}",
        data={
            **params,
            "access_token": VK_COMMUNITY_TOKEN,
            "v": VK_API_VERSION,
        },
        timeout=15,
    )
    data = response.json()
    if "error" in data:
        raise HTTPException(status_code=502, detail=data["error"])
    return data["response"]


def build_shift_keyboard(shift_id: int, open_positions: list[ShiftPosition]) -> str:
    buttons = []
    for open_position in open_positions:
        buttons.append(
            [
                {
                    "action": {
                        "type": "callback",
                        "label": f"Хочу выйти: {open_position.position}",
                        "payload": json.dumps(
                            {
                                "type": "shift_response",
                                "shift_id": shift_id,
                                "position": open_position.position,
                            },
                            ensure_ascii=False,
                        ),
                    },
                    "color": "positive",
                }
            ]
        )

    return json.dumps(
        {
            "one_time": False,
            "inline": True,
            "buttons": buttons,
        },
        ensure_ascii=False,
    )


def save_shift_response(vk_id: int, payload: dict[str, Any], raw_payload: dict[str, Any]) -> None:
    with db_connection() as connection:
        connection.execute(
            """
            INSERT INTO shift_responses (
                shift_id, vk_id, position_name, response_status, response_message, raw_payload
            ) VALUES (?, ?, ?, ?, ?, ?)
            """,
            (
                payload.get("shift_id"),
                str(vk_id),
                payload.get("position", ""),
                "new",
                "Отклик получен от VK-бота",
                json.dumps(raw_payload, ensure_ascii=False),
            ),
        )


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.post("/vk/callback")
async def vk_callback(request: Request) -> str:
    event = await request.json()

    if VK_SECRET_KEY and event.get("secret") != VK_SECRET_KEY:
        raise HTTPException(status_code=403, detail="Invalid VK secret key")

    event_type = event.get("type")
    if event_type == "confirmation":
        return VK_CONFIRMATION_CODE

    if event_type == "message_event":
        obj = event.get("object", {})
        payload = obj.get("payload") or {}
        if isinstance(payload, str):
            payload = json.loads(payload)

        if payload.get("type") == "shift_response":
            vk_id = obj.get("user_id")
            if vk_id:
                save_shift_response(vk_id, payload, event)

            event_id = obj.get("event_id")
            peer_id = obj.get("peer_id")
            if event_id and peer_id:
                vk_api(
                    "messages.sendMessageEventAnswer",
                    {
                        "event_id": event_id,
                        "user_id": vk_id,
                        "peer_id": peer_id,
                        "event_data": json.dumps(
                            {
                                "type": "show_snackbar",
                                "text": "Отклик отправлен администратору",
                            },
                            ensure_ascii=False,
                        ),
                    },
                )

    return "ok"


@app.post("/api/send-shift-notification")
def send_shift_notification(payload: ShiftNotificationRequest) -> dict[str, Any]:
    keyboard = build_shift_keyboard(payload.shift_id, payload.open_positions)
    results = []

    for user_id in payload.user_ids:
        status = "sent"
        error_text = ""
        try:
            vk_api(
                "messages.send",
                {
                    "user_id": user_id,
                    "random_id": random.randint(1, 2_147_483_647),
                    "message": payload.message,
                    "keyboard": keyboard,
                },
            )
        except HTTPException as exc:
            status = "error"
            error_text = str(exc.detail)

        with db_connection() as connection:
            connection.execute(
                """
                INSERT INTO sent_notifications (
                    shift_id, user_id, message_text, send_status, error_text
                ) VALUES (?, ?, ?, ?, ?)
                """,
                (payload.shift_id, user_id, payload.message, status, error_text),
            )

        results.append({"user_id": user_id, "status": status, "error": error_text})

    return {"results": results}


@app.get("/api/shift-responses")
def get_shift_responses() -> dict[str, list[dict[str, Any]]]:
    with db_connection() as connection:
        rows = connection.execute(
            """
            SELECT id, shift_id, vk_id, position_name, response_status,
                   response_message, created_at, processed_at
            FROM shift_responses
            ORDER BY created_at DESC, id DESC
            """
        ).fetchall()

    return {"responses": [dict(row) for row in rows]}
