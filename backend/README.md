# VK Bot Backend

Минимальный backend для VK-бота дипломного проекта.

## Что делает backend

- Принимает события VK Callback API на `POST /vk/callback`.
- Отвечает кодом подтверждения на событие `confirmation`.
- Принимает нажатие callback-кнопки `Хочу выйти`.
- Сохраняет отклики сотрудников в локальную SQLite-базу `backend.db`.
- Отправляет уведомление о смене через VK API методом `messages.send`.
- Отдаёт сохранённые отклики desktop-приложению через `GET /api/shift-responses`.

## Установка

```powershell
cd C:\Users\Dmitrii\Documents\VKR_2\backend
python -m venv .venv
.\.venv\Scripts\activate
pip install -r requirements.txt
```

## Настройка

Скопируй пример настроек:

```powershell
copy .env.example .env
```

Заполни `.env`:

```env
VK_GROUP_ID=123456789
VK_COMMUNITY_TOKEN=токен_сообщества
VK_CONFIRMATION_CODE=код_подтверждения_callback_api
VK_SECRET_KEY=секретный_ключ_из_vk
VK_API_VERSION=5.199
BACKEND_DB_PATH=backend.db
```

Важно: файл `.env` нельзя коммитить в GitHub.

## Запуск

```powershell
uvicorn app:app --reload --host 127.0.0.1 --port 8000
```

Проверка:

```text
http://127.0.0.1:8000/health
```

## Как подключить VK Callback API

Для локальной разработки VK нужен публичный HTTPS-адрес. Удобный вариант:

```powershell
ngrok http 8000
```

В настройках VK Callback API укажи URL:

```text
https://адрес-ngrok/vk/callback
```

Когда VK отправит событие `confirmation`, backend вернёт `VK_CONFIRMATION_CODE`.

## Отправка тестового уведомления

После запуска backend можно отправить POST-запрос на:

```text
POST http://127.0.0.1:8000/api/send-shift-notification
```

Пример JSON:

```json
{
  "shift_id": 1,
  "message": "Открыта новая смена. Нужен пекарь.",
  "user_ids": [123456789],
  "open_positions": [
    {
      "position": "Пекарь",
      "count": 1
    }
  ]
}
```

Сотрудник получит сообщение с кнопкой `Хочу выйти: Пекарь`.
