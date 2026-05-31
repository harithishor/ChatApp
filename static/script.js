let ws = null;
let currentUser = '';

function now_time() {
    return new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

function joinChat() {
    const input = document.getElementById('username');
    const name = input.value.trim();
    if (!name) {
        input.focus();
        input.style.borderColor = '#e24b4a';
        setTimeout(() => input.style.borderColor = '', 800);
        return;
    }
    currentUser = name;
    document.getElementById('current-user').textContent = name;
    document.getElementById('login-screen').style.display = 'none';
    document.getElementById('chat-screen').style.display = 'grid';
    connectWebSocket(name);
}

function connectWebSocket(username) {
    const status = document.getElementById('connection-status');
    const protocol = location.protocol === 'https:' ? 'wss' : 'ws';
    ws = new WebSocket(`ws://localhost:8765`);

    ws.onopen = () => {
        status.textContent = 'connected';
        status.classList.add('live');
        ws.send(JSON.stringify({ type: 'join', username }));
    };

    ws.onmessage = (e) => {
        try {
            const data = JSON.parse(e.data);
            if (data.type === 'history') {
                for (const m of data.messages) {
                    appendMessage(m.username, m.text, m.time, false, true);
                }
            }
            else if (data.type === 'message') {
                // Always show messages from server (they are from OTHER users)
                // because we already show our own instantly on send
                appendMessage(data.username, data.text, data.time, false, false);
            } else if (data.type === 'system') {
                appendSystem(data.text);
            }
        } catch (err) {
            console.error('Parse error:', err, e.data);
        }
    };

    ws.onclose = () => {
        status.textContent = 'disconnected';
        status.classList.remove('live');
        appendSystem('disconnected from server');
    };

    ws.onerror = (err) => {
        status.textContent = 'error';
        appendSystem('connection error — is the C++ server running?');
    };
}

function sendMessage() {
    const input = document.getElementById('msg');
    const text = input.value.trim();
    if (!text) return;
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        appendSystem('not connected — refresh and try again');
        return;
    }
    // Show own message immediately on our side
    appendMessage(currentUser, text, now_time(), true, false);
    ws.send(JSON.stringify({ type: 'message', text }));
    input.value = '';
    input.focus();
}

let liveDividerAdded = false;

function appendMessage(username, text, time, isOwn, isHistory) {
    if (!isHistory && !liveDividerAdded) {
        addDivider('live');
        liveDividerAdded = true;
    }
    const area = document.getElementById('messages');
    const row = document.createElement('div');
    row.className = `msg-row ${isOwn ? 'own' : 'other'}`;
    const t = time || now_time();
    row.innerHTML = `
    <div class="msg-meta">
      <span class="msg-author">${escHtml(username)}</span>
      <span class="msg-time">${t}</span>
    </div>
    <div class="msg-bubble">${escHtml(text)}</div>
  `;
    area.appendChild(row);
    area.scrollTop = area.scrollHeight;
}

function appendSystem(text) {
    const area = document.getElementById('messages');
    const div = document.createElement('div');
    div.className = 'msg-system';
    div.textContent = `— ${text} —`;
    area.appendChild(div);
    area.scrollTop = area.scrollHeight;
}

function addDivider(label) {
    const area = document.getElementById('messages');
    const div = document.createElement('div');
    div.className = 'history-divider';
    div.textContent = label;
    area.appendChild(div);
}

function escHtml(str) {
    return str
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}

document.addEventListener('DOMContentLoaded', () => {
    document.getElementById('msg')?.addEventListener('keydown', e => {
        if (e.key === 'Enter') sendMessage();
    });
    document.getElementById('username')?.addEventListener('keydown', e => {
        if (e.key === 'Enter') joinChat();
    });
});