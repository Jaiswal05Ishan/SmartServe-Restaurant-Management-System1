const API_BASE = "http://127.0.0.1:5000/api";

let foods = [];
let orders = [];
let payments = {};
let toastTimer;

const $ = id => document.getElementById(id);
const money = value => "₹" + Number(value || 0).toLocaleString("en-IN", {
  minimumFractionDigits: 2, maximumFractionDigits: 2
});

function toast(message) {
  const node = $("toast");
  if (!node) { alert(message); return; }
  node.textContent = message;
  node.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => node.classList.remove("show"), 2500);
}

function foodById(id) {
  return foods.find(food => Number(food.id) === Number(id));
}

function normalizeOrder(order) {
  return {
    id: order.id,
    foodId: order.foodId,
    qty: order.quantity,
    total: Number(order.total),
    processed: order.status === "processed"
  };
}

async function api(path, options = {}) {
  const response = await fetch(`${API_BASE}${path}`, {
    ...options,
    headers: { "Content-Type": "application/json", ...(options.headers || {}) }
  });
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(data.error || data.message || `Request failed (${response.status})`);
  return data;
}

function setConnectionStatus(text, connected) {
  // Update the visible demo/status pill without depending on a specific class.
  const candidates = document.querySelectorAll(
    ".top-status, .today-pill, .status-pill, .header-status, header .pill, .topbar .pill"
  );
  candidates.forEach(node => {
    if (node.textContent.toLowerCase().includes("demo") ||
        node.textContent.toLowerCase().includes("live") ||
        node.textContent.toLowerCase().includes("api")) {
      node.textContent = text;
      node.style.color = connected ? "#087f5b" : "#b42318";
    }
  });
  const notice = document.querySelector(".preview-note, .frontend-preview, .notice");
  if (notice && notice.textContent.toLowerCase().includes("frontend preview")) {
    notice.textContent = connected
      ? "Connected to the SmartServe backend. Orders are loaded from the server."
      : "Backend connection unavailable. Start backend/app.py and refresh.";
  }
}

async function loadAll(showToast = false) {
  try {
    const [menuData, orderData] = await Promise.all([
      api("/menu"),
      api("/orders")
    ]);
    foods = menuData;
    orders = (orderData.orders || []).map(normalizeOrder);
    renderFood();
    renderAll();
    setConnectionStatus("Live System", true);
    if (showToast) toast("Live data refreshed.");
  } catch (error) {
    console.error(error);
    setConnectionStatus("API Disconnected", false);
    toast("Cannot connect to Flask. Start backend/app.py and refresh.");
  }
}

function statusBadge(order) {
  const status = order.processed ? "processed" : "pending";
  return `<span class="status ${status}">${order.processed ? "Processed" : "Pending"}</span>`;
}

function orderRow(order, withAction = false) {
  const food = foodById(order.foodId);
  return `<tr>
    <td>#${order.id}</td>
    <td>${food?.name || "Unknown item"}</td>
    <td>${order.qty}</td>
    <td>${money(order.total)}</td>
    <td>${statusBadge(order)}</td>
    ${withAction ? `<td>${order.processed ? '<span class="muted">Complete</span>' :
      `<button type="button" class="text-btn process-one" data-id="${order.id}">Process next</button>`}</td>` : ""}
  </tr>`;
}

function renderDashboard() {
  const pending = orders.filter(o => !o.processed);
  const processed = orders.filter(o => o.processed);
  const total = orders.reduce((sum, o) => sum + o.total, 0);

  if ($("total-orders")) $("total-orders").textContent = orders.length;
  if ($("pending-orders")) $("pending-orders").textContent = pending.length;
  if ($("processed-orders")) $("processed-orders").textContent = processed.length;
  if ($("order-value")) $("order-value").textContent = money(total);

  if ($("recent-orders")) {
    $("recent-orders").innerHTML = orders.slice().reverse().slice(0, 5)
      .map(o => orderRow(o)).join("") ||
      '<tr><td colspan="5">No orders yet</td></tr>';
  }
}

function ensureAddFoodForm() {
  const grid = $("food-grid");
  if (!grid || $("add-food-form")) return;

  const wrapper = document.createElement("section");
  wrapper.className = "food-card";
  wrapper.style.marginBottom = "18px";
  wrapper.innerHTML = `
    <h3>Add a food item</h3>
    <form id="add-food-form" style="display:grid;gap:10px;max-width:520px">
      <input id="new-food-name" type="text" placeholder="Food name" required maxlength="60"
        style="padding:10px;border:1px solid #ddd;border-radius:8px">
      <input id="new-food-price" type="number" placeholder="Price in ₹" required min="1" step="0.01"
        style="padding:10px;border:1px solid #ddd;border-radius:8px">
      <input id="new-food-desc" type="text" placeholder="Short description (optional)" maxlength="120"
        style="padding:10px;border:1px solid #ddd;border-radius:8px">
      <button type="submit" class="primary-btn">Add food item</button>
    </form>`;
  grid.parentNode.insertBefore(wrapper, grid);
  $("add-food-form").addEventListener("submit", addFood);
}

function renderFood() {
  if ($("food-grid")) {
    $("food-grid").innerHTML = foods.map(food => `
      <article class="food-card">
        <div class="food-emoji">${food.emoji || "🍽️"}</div>
        <h3>${escapeHtml(food.name)}</h3>
        <p>${escapeHtml(food.desc || "Freshly prepared")}</p>
        <strong>${money(food.price)}</strong>
      </article>`).join("");
    ensureAddFoodForm();
  }

  if ($("food-select")) {
    const select = $("food-select");
    const oldValue = select.value;
    select.innerHTML = foods.map(food =>
      `<option value="${food.id}">${escapeHtml(food.name)} — ${money(food.price)}</option>`
    ).join("");
    if (foods.some(f => String(f.id) === oldValue)) select.value = oldValue;
    updateFormTotal();
  }
}

function escapeHtml(value) {
  return String(value ?? "").replace(/[&<>"']/g, ch => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  })[ch]);
}

async function addFood(event) {
  event.preventDefault();
  const name = $("new-food-name").value.trim();
  const price = Number($("new-food-price").value);
  const desc = $("new-food-desc").value.trim();

  if (!name || !Number.isFinite(price) || price <= 0) {
    toast("Enter a valid food name and price.");
    return;
  }

  try {
    await api("/menu", {
      method: "POST",
      body: JSON.stringify({ name, price, desc })
    });
    await loadAll();
    toast("Food item added and saved.");
  } catch (error) {
    toast(error.message);
  }
}

function renderOrders() {
  if ($("all-orders")) {
    $("all-orders").innerHTML = orders.map(o => orderRow(o, true)).join("") ||
      '<tr><td colspan="6">No orders yet</td></tr>';
  }
  if ($("bill-order")) {
    const select = $("bill-order");
    const oldValue = select.value;
    select.innerHTML = orders.map(o =>
      `<option value="${o.id}">Order #${o.id} — ${escapeHtml(foodById(o.foodId)?.name || "Unknown item")}</option>`
    ).join("");
    if (orders.some(o => String(o.id) === oldValue)) select.value = oldValue;
  }
  renderBill();
}

function renderQueue() {
  const pending = orders.filter(o => !o.processed);
  if ($("queue-count")) $("queue-count").textContent =
    `${pending.length} order${pending.length === 1 ? "" : "s"} waiting`;
  if ($("queue-list")) {
    $("queue-list").innerHTML = pending.length ? pending.map((o, i) => `
      <div class="queue-item">
        <div class="queue-number">${i + 1}</div>
        <div class="queue-info">
          <strong>Order #${o.id} · ${escapeHtml(foodById(o.foodId)?.name || "Unknown item")}</strong>
          <small>Quantity: ${o.qty} · FIFO position ${i + 1}</small>
        </div>
        <span class="queue-price">${money(o.total)}</span>
      </div>`).join("") : '<p class="muted">Kitchen queue is empty.</p>';
  }
}

function renderDSA() {
  const pending = orders.filter(o => !o.processed);
  if ($("dsa-queue")) $("dsa-queue").innerHTML = pending.map(o =>
    `<div class="dsa-node">#${o.id} · ${escapeHtml(foodById(o.foodId)?.name || "Unknown item")}</div>`
  ).join("") || '<span class="muted">Queue empty</span>';

  if ($("dsa-stack")) $("dsa-stack").innerHTML = orders.slice().reverse().map(o =>
    `<div class="dsa-node">TOP → #${o.id} · ${escapeHtml(foodById(o.foodId)?.name || "Unknown item")}</div>`
  ).join("") || '<span class="muted">Stack empty</span>';

  if ($("dsa-list")) $("dsa-list").innerHTML = orders.map((o, i) =>
    `${i ? '<span class="chain-arrow">→</span>' : ""}<span class="chain-node">#${o.id}</span>`
  ).join("") + '<span class="chain-arrow"> → NULL</span>';
}

function renderAll() {
  renderDashboard();
  renderOrders();
  renderQueue();
  renderDSA();
}

function updateFormTotal() {
  if (!$("food-select") || !$("quantity") || !$("form-total")) return;
  const food = foodById($("food-select").value);
  const qty = Math.max(1, Number($("quantity").value) || 1);
  $("form-total").textContent = money(food ? food.price * qty : 0);
}

function renderBill() {
  if (!$("bill-order") || !$("bill-details")) return;
  const order = orders.find(o => o.id === Number($("bill-order").value));
  const target = $("bill-details");

  if (!order) {
    target.innerHTML = '<p class="muted">No orders available to bill.</p>';
    return;
  }

  const food = foodById(order.foodId);
  const gst = order.total * 0.05; // Example only; confirm applicable tax rate.
  const total = order.total + gst;
  const payment = payments[order.id];

  target.innerHTML = `
    <div class="bill-line"><span>Order</span><strong>#${order.id}</strong></div>
    <div class="bill-line"><span>${escapeHtml(food?.name || "Item")} × ${order.qty}</span><strong>${money(order.total)}</strong></div>
    <div class="bill-line"><span>Subtotal</span><strong>${money(order.total)}</strong></div>
    <div class="bill-line"><span>GST (sample 5%)</span><strong>${money(gst)}</strong></div>
    <div class="bill-line total"><span>Total payable</span><strong>${money(total)}</strong></div>
    <div class="bill-line" style="display:flex;gap:12px;align-items:center;flex-wrap:wrap">
      <label for="payment-method">Payment method</label>
      <select id="payment-method" ${payment ? "disabled" : ""}>
        <option value="Cash">Cash</option><option value="UPI">UPI</option><option value="Card">Card</option>
      </select>
    </div>
    ${payment ? `<p class="status processed">Payment recorded: ${escapeHtml(payment.method)}</p>` :
      '<button type="button" id="record-payment" class="primary-btn">Record payment</button>'}
    <p class="muted">Project payment record only. No money is transferred. GST is illustrative.</p>`;
}

function recordPayment() {
  const order = orders.find(o => o.id === Number($("bill-order").value));
  if (!order) return toast("Select an order first.");
  if (payments[order.id]) return toast("Payment already recorded for this order.");
  const method = $("payment-method")?.value || "Cash";
  payments[order.id] = { method, recordedAt: new Date().toISOString() };
  renderBill();
  toast(`Payment recorded by ${method}.`);
}

function go(page) {
  document.querySelectorAll(".page").forEach(node =>
    node.classList.toggle("active", node.id === page));
  document.querySelectorAll(".nav-item").forEach(node =>
    node.classList.toggle("active", node.dataset.page === page));
  const titles = {
    dashboard: "Dashboard", menu: "Food menu", orders: "Orders",
    kitchen: "Kitchen queue", billing: "Billing", "data-structures": "DSA lab"
  };
  if ($("page-title")) $("page-title").textContent = titles[page] || "Dashboard";
  window.scrollTo({ top: 0, behavior: "smooth" });
}

document.querySelectorAll(".nav-item").forEach(button =>
  button.addEventListener("click", () => go(button.dataset.page)));
document.querySelectorAll("[data-go]").forEach(button =>
  button.addEventListener("click", () => go(button.dataset.go)));

if ($("food-select")) $("food-select").addEventListener("change", updateFormTotal);
if ($("quantity")) $("quantity").addEventListener("input", updateFormTotal);
if ($("bill-order")) $("bill-order").addEventListener("change", renderBill);

if ($("order-form")) $("order-form").addEventListener("submit", async event => {
  event.preventDefault();
  const foodId = Number($("food-select").value);
  const quantity = Number($("quantity").value);
  if (!foodById(foodId) || !Number.isInteger(quantity) || quantity < 1 || quantity > 100) {
    return toast("Enter a quantity from 1 to 100.");
  }
  try {
    await api("/orders", { method: "POST", body: JSON.stringify({ foodId, quantity }) });
    await loadAll();
    go("orders");
    toast("Order saved to the backend.");
  } catch (error) { toast(error.message); }
});

if ($("process-next")) $("process-next").addEventListener("click", async () => {
  try {
    const result = await api("/orders/process-next", { method: "POST" });
    await loadAll();
    toast(result.message || "Kitchen queue updated.");
  } catch (error) { toast(error.message); }
});

if ($("all-orders")) $("all-orders").addEventListener("click", async event => {
  if (!event.target.closest(".process-one")) return;
  try {
    const result = await api("/orders/process-next", { method: "POST" });
    await loadAll();
    toast(result.message || "Kitchen queue updated in FIFO order.");
  } catch (error) { toast(error.message); }
});

if ($("bill-details")) $("bill-details").addEventListener("click", event => {
  if (event.target.closest("#record-payment")) recordPayment();
});

const refreshButton = $("reset-demo");
if (refreshButton) {
  refreshButton.textContent = "Refresh live data";
  refreshButton.addEventListener("click", () => loadAll(true));
}

if ($("peek-stack")) $("peek-stack").addEventListener("click", () => {
  const order = orders[orders.length - 1];
  if ($("stack-message")) $("stack-message").textContent = order
    ? `Latest order is #${order.id} (${foodById(order.foodId)?.name || "Unknown item"}).`
    : "Stack is empty.";
});

if ($("bst-search-btn")) $("bst-search-btn").addEventListener("click", () => {
  const id = Number($("bst-search")?.value);
  const order = orders.find(o => o.id === id);
  if ($("bst-result")) $("bst-result").textContent = order
    ? `Found #${order.id}: ${foodById(order.foodId)?.name || "Unknown item"}, quantity ${order.qty}, ${money(order.total)}, ${order.processed ? "Processed" : "Pending"}.`
    : "Order ID not found.";
});

renderFood();
renderAll();
loadAll();
