from flask import Flask, jsonify, request
from flask_cors import CORS
from pathlib import Path
import json
import os
import tempfile

app = Flask(__name__)
CORS(app)

BASE_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = BASE_DIR / "data"
ORDERS_FILE = DATA_DIR / "orders.txt"
MENU_FILE = DATA_DIR / "menu.json"
MAX_ORDERS = 100

DEFAULT_MENU = [
    {"id": 1, "name": "Paneer Tikka", "price": 180.0, "emoji": "🍢", "desc": "Smoky, marinated paneer"},
    {"id": 2, "name": "Veg Biryani", "price": 160.0, "emoji": "🍚", "desc": "Fragrant rice and spices"},
    {"id": 3, "name": "Masala Dosa", "price": 90.0, "emoji": "🥞", "desc": "Crispy dosa with masala"},
    {"id": 4, "name": "Veg Burger", "price": 110.0, "emoji": "🍔", "desc": "Veg patty and fresh greens"},
    {"id": 5, "name": "Cold Coffee", "price": 80.0, "emoji": "🥤", "desc": "Chilled coffee"},
    {"id": 6, "name": "Gulab Jamun", "price": 60.0, "emoji": "🍮", "desc": "Classic sweet dessert"},
]

def atomic_write(path, content):
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    fd, temp_path = tempfile.mkstemp(prefix="tmp_", suffix=".tmp", dir=str(DATA_DIR), text=True)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as file:
            file.write(content)
        os.replace(temp_path, path)
    finally:
        if os.path.exists(temp_path):
            os.remove(temp_path)

def load_menu():
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    if not MENU_FILE.exists():
        atomic_write(MENU_FILE, json.dumps(DEFAULT_MENU, ensure_ascii=False, indent=2))
    with MENU_FILE.open("r", encoding="utf-8") as file:
        menu = json.load(file)
    if not isinstance(menu, list):
        raise ValueError("Menu data must be a list")
    return menu

def save_menu(menu):
    atomic_write(MENU_FILE, json.dumps(menu, ensure_ascii=False, indent=2))

def load_orders():
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    menu = load_menu()
    orders = []
    if not ORDERS_FILE.exists():
        return orders
    with ORDERS_FILE.open("r", encoding="utf-8") as file:
        for line_number, line in enumerate(file, start=1):
            if not line.strip():
                continue
            parts = line.split()
            if len(parts) != 5:
                raise ValueError(f"Invalid order record on line {line_number}")
            order_id, food_id, quantity = map(int, parts[:3])
            total = float(parts[3])
            processed = int(parts[4])
            food = next((item for item in menu if int(item["id"]) == food_id), None)
            if food is None or order_id <= 0 or quantity <= 0 or total < 0 or processed not in (0, 1):
                raise ValueError(f"Invalid order data on line {line_number}")
            orders.append({
                "id": order_id, "foodId": food_id, "food": food["name"],
                "price": float(food["price"]), "quantity": quantity,
                "total": round(total, 2), "status": "processed" if processed else "pending"
            })
    if len(orders) > MAX_ORDERS:
        raise ValueError("Maximum order limit exceeded")
    if len({o["id"] for o in orders}) != len(orders):
        raise ValueError("Duplicate order IDs found")
    return orders

def save_orders(orders):
    lines = []
    for order in orders:
        processed = 1 if order["status"] == "processed" else 0
        lines.append(f'{order["id"]} {order["foodId"]} {order["quantity"]} {order["total"]:.2f} {processed}')
    atomic_write(ORDERS_FILE, "\n".join(lines) + ("\n" if lines else ""))

@app.get("/")
def home():
    return jsonify({"message": "SmartServe API is running", "menu_endpoint": "/api/menu", "orders_endpoint": "/api/orders"})

@app.get("/api/health")
def health():
    return jsonify({"status": "ok", "service": "SmartServe API"})

@app.get("/api/menu")
def get_menu():
    try:
        return jsonify(load_menu())
    except (OSError, ValueError, json.JSONDecodeError) as error:
        return jsonify({"error": str(error)}), 500

@app.post("/api/menu")
def add_menu_item():
    data = request.get_json(silent=True) or {}
    name = str(data.get("name", "")).strip()
    desc = str(data.get("desc", "")).strip()
    try:
        price = float(data.get("price", 0))
    except (TypeError, ValueError):
        return jsonify({"error": "Price must be a number"}), 400
    if not name or len(name) > 60:
        return jsonify({"error": "Food name is required (maximum 60 characters)"}), 400
    if price <= 0:
        return jsonify({"error": "Price must be greater than zero"}), 400
    try:
        menu = load_menu()
        if any(item["name"].casefold() == name.casefold() for item in menu):
            return jsonify({"error": "This food item already exists"}), 409
        new_item = {
            "id": max((int(item["id"]) for item in menu), default=0) + 1,
            "name": name, "price": round(price, 2),
            "emoji": "🍽️", "desc": desc or "Freshly prepared"
        }
        menu.append(new_item)
        save_menu(menu)
        return jsonify({"message": "Food item added", "item": new_item}), 201
    except (OSError, ValueError, json.JSONDecodeError) as error:
        return jsonify({"error": str(error)}), 500

@app.get("/api/orders")
def get_orders():
    try:
        orders = load_orders()
        return jsonify({
            "orders": orders, "totalOrders": len(orders),
            "pendingOrders": sum(o["status"] == "pending" for o in orders),
            "processedOrders": sum(o["status"] == "processed" for o in orders)
        })
    except (OSError, ValueError, json.JSONDecodeError) as error:
        return jsonify({"error": str(error)}), 500

@app.post("/api/orders")
def create_order():
    data = request.get_json(silent=True) or {}
    try:
        food_id = int(data.get("foodId", 0))
        quantity = int(data.get("quantity", 0))
    except (TypeError, ValueError):
        return jsonify({"error": "Food ID and quantity must be integers"}), 400
    try:
        menu = load_menu()
        food = next((item for item in menu if int(item["id"]) == food_id), None)
        if food is None:
            return jsonify({"error": "Invalid food ID"}), 400
        if quantity < 1 or quantity > 100:
            return jsonify({"error": "Quantity must be between 1 and 100"}), 400
        orders = load_orders()
        if len(orders) >= MAX_ORDERS:
            return jsonify({"error": "Maximum order limit reached"}), 400
        order_id = max((o["id"] for o in orders), default=0) + 1
        order = {
            "id": order_id, "foodId": food_id, "food": food["name"],
            "price": float(food["price"]), "quantity": quantity,
            "total": round(float(food["price"]) * quantity, 2), "status": "pending"
        }
        orders.append(order)
        save_orders(orders)
        return jsonify({"message": "Order saved successfully", "order": order}), 201
    except (OSError, ValueError, json.JSONDecodeError) as error:
        return jsonify({"error": str(error)}), 500

@app.post("/api/orders/process-next")
def process_next_order():
    try:
        orders = load_orders()
        pending = [o for o in orders if o["status"] == "pending"]
        if not pending:
            return jsonify({"message": "No pending orders"}), 200
        next_order = min(pending, key=lambda o: o["id"])
        next_order["status"] = "processed"
        save_orders(orders)
        return jsonify({"message": "Kitchen order processed", "order": next_order})
    except (OSError, ValueError, json.JSONDecodeError) as error:
        return jsonify({"error": str(error)}), 500

@app.get("/api/orders/<int:order_id>")
def get_order(order_id):
    try:
        orders = load_orders()
        order = next((o for o in orders if o["id"] == order_id), None)
        if order is None:
            return jsonify({"error": "Order not found"}), 404
        return jsonify(order)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        return jsonify({"error": str(error)}), 500

if __name__ == "__main__":
    print("SmartServe API starting...")
    print(f"Orders file: {ORDERS_FILE}")
    print(f"Menu file: {MENU_FILE}")
    app.run(host="127.0.0.1", port=5000, debug=False)
