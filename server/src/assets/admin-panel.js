    //  State 
    let TOKEN = localStorage.getItem('admin_token') || '';
    let CHEATS = [], USERS = [], SESSIONS = [], RESETS = [];
    let editingCheatId = null;
    let wizardStep = 0;

    const API = '/api/admin';
    const headers = () => ({ 'Authorization': `Bearer ${TOKEN}`, 'Content-Type': 'application/json' });

    //  Auth 
    async function doLogin() {
      const u = document.getElementById('loginUser').value.trim();
      const p = document.getElementById('loginPass').value;
      const errEl = document.getElementById('loginError');
      if (!u || !p) { errEl.textContent = 'Fill in all fields'; errEl.style.display = 'block'; return; }
      try {
        const r = await fetch(`${API}/login`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ username: u, password: p }) });
        const d = await r.json();
        if (!d.success) { errEl.textContent = d.error; errEl.style.display = 'block'; return; }
        TOKEN = d.token;
        localStorage.setItem('admin_token', TOKEN);
        document.getElementById('adminName').textContent = d.nickname || u;
        document.getElementById('userAvatar').textContent = (d.nickname || u).charAt(0).toUpperCase();
        showDashboard();
      } catch (e) { errEl.textContent = 'Connection error'; errEl.style.display = 'block'; }
    }

    function doLogout() {
      TOKEN = '';
      localStorage.removeItem('admin_token');
      document.getElementById('loginPage').style.display = 'flex';
      document.getElementById('dashboard').style.display = 'none';
    }

    async function showDashboard() {
      document.getElementById('loginPage').style.display = 'none';
      document.getElementById('dashboard').style.display = 'block';
      await Promise.all([loadCheats(), loadUsers(), loadSessions(), loadResets()]);
    }

    //  Auto-login 
    if (TOKEN) {
      fetch(`${API}/cheats`, { headers: headers() }).then(r => {
        if (r.ok) showDashboard();
        else doLogout();
      }).catch(() => doLogout());
    }

    document.getElementById('loginPass').addEventListener('keydown', e => { if (e.key === 'Enter') doLogin(); });

    //  Navigation 
    document.querySelectorAll('.nav-item').forEach(item => {
      item.addEventListener('click', () => {
        document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
        item.classList.add('active');
        document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
        document.getElementById('sec-' + item.dataset.section).classList.add('active');
      });
    });

    //  API Helpers 
    async function apiFetch(url, opts = {}) {
      opts.headers = { ...headers(), ...opts.headers };
      const r = await fetch(url, opts);
      if (r.status === 401 || r.status === 403) { doLogout(); throw new Error('Auth expired'); }
      return r;
    }

    function toast(msg, type = 'success') {
      const t = document.createElement('div');
      t.className = `toast toast-${type}`;
      t.textContent = msg;
      document.getElementById('toastContainer').appendChild(t);
      setTimeout(() => t.remove(), 4000);
    }

    function openModal(id) { document.getElementById(id).classList.add('show'); }
    function closeModal(id) { document.getElementById(id).classList.remove('show'); }

    function confirmAction(title, msg, onConfirm) {
      document.getElementById('confirmTitle').textContent = title;
      document.getElementById('confirmMsg').textContent = msg;
      document.getElementById('confirmBtn').onclick = () => { closeModal('confirmModal'); onConfirm(); };
      openModal('confirmModal');
    }

    //  Cheats 
    async function loadCheats() {
      try {
        const r = await apiFetch(`${API}/cheats`);
        const d = await r.json();
        if (d.success) { CHEATS = d.cheats; renderCheats(); }
      } catch (e) { console.error(e); }
    }

    function renderCheats() {
      document.getElementById('statCheats').textContent = CHEATS.length;
      document.getElementById('cheatCount').textContent = CHEATS.length;
      const grid = document.getElementById('cheatsGrid');
      const q = document.getElementById('cheatSearch').value.toLowerCase();
      const filtered = CHEATS.filter(c => !q || c.game.toLowerCase().includes(q) || c.name.toLowerCase().includes(q));

      if (filtered.length === 0) {
        grid.innerHTML = '<div class="empty-state"><div class="icon"></div><p>No cheats found</p></div>';
        return;
      }

      grid.innerHTML = filtered.map(c => `
        <div class="cheat-card">
            <div class="cheat-accent" style="background:${c.icon_color || '#00e68a'}"></div>
            <div class="cheat-card-body">
                <div class="cheat-card-top">
                    <div class="cheat-icon" style="background:${c.icon_color || '#00e68a'}">${(c.game || '?').substring(0, 2).toUpperCase()}</div>
                    <div class="cheat-meta">
                        <h4>${esc(c.name)}</h4>
                        <div class="game">${esc(c.game)}</div>
                    </div>
                </div>
                <div class="cheat-badges">
                    <span class="badge ${c.enabled ? 'badge-on' : 'badge-off'}">${c.enabled ? 'ON' : 'OFF'}</span>
                    ${c.payload_path ? '<span class="badge badge-payload">Payload OK</span>' : ''}
                    ${c.requires_admin ? '<span class="badge badge-admin">Admin</span>' : ''}
                    <span class="badge badge-method">${c.injection_method || 'LoadLibrary'}</span>
                </div>
                <div class="cheat-card-actions">
                    <button class="btn btn-blue btn-sm" onclick="editCheat(${c.id})" style="flex:1">Edit</button>
                    <button class="btn btn-danger btn-sm" onclick="deleteCheat(${c.id},'${esc(c.name)}')" style="flex:1">Delete</button>
                </div>
            </div>
        </div>
    `).join('');
    }

    function filterCheats() { renderCheats(); }

    //  Cheat Wizard 
    function openCheatWizard(cheat = null) {
      editingCheatId = cheat ? cheat.id : null;
      document.getElementById('cheatModalTitle').textContent = cheat ? 'Edit Cheat' : 'New Cheat';
      document.getElementById('cGame').value = cheat ? cheat.game : '';
      document.getElementById('cName').value = cheat ? cheat.name : '';
      document.getElementById('cColor').value = cheat ? (cheat.icon_color || '#00e68a') : '#00e68a';
      document.getElementById('cColorHex').textContent = document.getElementById('cColor').value;
      document.getElementById('cEnabled').value = cheat ? (cheat.enabled ? '1' : '0') : '1';
      document.getElementById('cNotes').value = cheat ? (cheat.notes || '') : '';
      document.getElementById('cProcess').value = cheat ? (cheat.process_name || '') : '';
      document.getElementById('cMethod').value = cheat ? (cheat.injection_method || 'LoadLibrary') : 'LoadLibrary';
      document.getElementById('cKill').value = cheat ? (cheat.kill_processes || '') : '';
      document.getElementById('cLaunch').value = cheat ? (cheat.launch_params || '') : '';
      document.getElementById('cSteam').value = cheat ? (cheat.steam_app_id || '') : '';
      document.getElementById('cAdmin').checked = cheat ? !!cheat.requires_admin : false;
      document.getElementById('cPayload').value = '';
      document.getElementById('fileName').textContent = cheat && cheat.payload_path ? `Current: ${cheat.payload_path}` : '';
      wizardStep = 0;
      updateWizard();
      openModal('cheatModal');
    }

    document.getElementById('cColor').addEventListener('input', e => {
      document.getElementById('cColorHex').textContent = e.target.value;
    });

    function wizardGo(step) { wizardStep = step; updateWizard(); }
    function wizardNext() { if (wizardStep < 2) { wizardStep++; updateWizard(); } }
    function wizardPrev() { if (wizardStep > 0) { wizardStep--; updateWizard(); } }

    function updateWizard() {
      document.querySelectorAll('.wizard-step').forEach((s, i) => {
        s.classList.toggle('active', i === wizardStep);
        s.classList.toggle('done', i < wizardStep);
      });
      document.querySelectorAll('.wizard-panel').forEach((p, i) => p.classList.toggle('active', i === wizardStep));
      document.getElementById('wizPrev').style.display = wizardStep > 0 ? '' : 'none';
      document.getElementById('wizNext').style.display = wizardStep < 2 ? '' : 'none';
      document.getElementById('wizSave').style.display = wizardStep === 2 ? '' : 'none';
    }

    function editCheat(id) {
      const c = CHEATS.find(x => x.id === id);
      if (c) openCheatWizard(c);
    }

    async function saveCheat() {
      const game = document.getElementById('cGame').value.trim();
      const name = document.getElementById('cName').value.trim();
      if (!game || !name) { toast('Game and Name are required', 'error'); wizardGo(0); return; }

      const fd = new FormData();
      fd.append('game', game);
      fd.append('name', name);
      fd.append('icon_color', document.getElementById('cColor').value);
      fd.append('enabled', document.getElementById('cEnabled').value);
      fd.append('notes', document.getElementById('cNotes').value);
      fd.append('process_name', document.getElementById('cProcess').value);
      fd.append('injection_method', document.getElementById('cMethod').value);
      fd.append('kill_processes', document.getElementById('cKill').value);
      fd.append('launch_params', document.getElementById('cLaunch').value);
      fd.append('steam_app_id', document.getElementById('cSteam').value);
      fd.append('requires_admin', document.getElementById('cAdmin').checked ? '1' : '0');

      const file = document.getElementById('cPayload').files[0];
      if (file) fd.append('payload', file);

      try {
        const url = editingCheatId ? `${API}/cheats/${editingCheatId}` : `${API}/cheats`;
        const method = editingCheatId ? 'PUT' : 'POST';
        const r = await fetch(url, { method, headers: { 'Authorization': `Bearer ${TOKEN}` }, body: fd });
        const d = await r.json();
        if (d.success) {
          toast(editingCheatId ? 'Cheat updated!' : 'Cheat created!');
          closeModal('cheatModal');
          await loadCheats();
        } else { toast(d.error || 'Error', 'error'); }
      } catch (e) { toast('Connection error', 'error'); }
    }

    function deleteCheat(id, name) {
      confirmAction('Delete Cheat', `Delete "${name}"? This will also remove all associated licenses.`, async () => {
        try {
          const r = await apiFetch(`${API}/cheats/${id}`, { method: 'DELETE' });
          const d = await r.json();
          if (d.success) { toast('Cheat deleted'); await loadCheats(); }
          else toast(d.error, 'error');
        } catch (e) { toast('Error', 'error'); }
      });
    }

    //  Drag & Drop 
    const dz = document.getElementById('dropZone');
    dz.addEventListener('dragover', e => { e.preventDefault(); dz.classList.add('dragover'); });
    dz.addEventListener('dragleave', () => dz.classList.remove('dragover'));
    dz.addEventListener('drop', e => {
      e.preventDefault(); dz.classList.remove('dragover');
      if (e.dataTransfer.files.length) {
        document.getElementById('cPayload').files = e.dataTransfer.files;
        handleFile(document.getElementById('cPayload'));
      }
    });

    function handleFile(input) {
      const f = input.files[0];
      document.getElementById('fileName').textContent = f ? ` ${f.name} (${(f.size / 1024 / 1024).toFixed(2)} MB)` : '';
    }

    //  Users 
    async function loadUsers() {
      try {
        const r = await apiFetch(`${API}/users`);
        const d = await r.json();
        if (d.success) { USERS = d.users; renderUsers(); }
      } catch (e) { console.error(e); }
    }

    function renderUsers() {
      document.getElementById('statUsers').textContent = USERS.length;
      document.getElementById('userCount').textContent = USERS.length;
      const q = document.getElementById('userSearch').value.toLowerCase();
      const filtered = USERS.filter(u => !q || u.username.toLowerCase().includes(q) || (u.nickname || '').toLowerCase().includes(q));
      const tb = document.getElementById('usersTable');

      if (filtered.length === 0) { tb.innerHTML = '<tr><td colspan="6"><div class="empty-state"><p>No users found</p></div></td></tr>'; return; }

      tb.innerHTML = filtered.map(u => {
        const planCls = u.plan === 'vip' ? 'plan-vip' : u.plan === 'premium' ? 'plan-premium' : 'plan-free';
        return `<tr>
            <td><div class="user-cell"><div class="user-avatar">${(u.nickname || u.username).charAt(0).toUpperCase()}</div><div><strong>${esc(u.nickname || u.username)}</strong><br><span style="font-size:11px;color:var(--text3)">${esc(u.username)}</span></div></div></td>
            <td><span class="plan-badge ${planCls}">${u.plan || 'free'}</span></td>
            <td><span class="${u.role === 'admin' ? 'role-admin' : 'role-user'}">${u.role || 'user'}</span></td>
            <td style="font-size:11px;color:var(--text3);max-width:160px;overflow:hidden;text-overflow:ellipsis">${u.hwid || ''}</td>
            <td style="font-size:12px;color:var(--text2)">${u.created_at ? new Date(u.created_at).toLocaleDateString() : ''}</td>
            <td><button class="btn btn-blue btn-sm" onclick="openGrant(${u.id},'${esc(u.nickname || u.username)}')"> Grant</button></td>
        </tr>`;
      }).join('');
    }

    function filterUsers() { renderUsers(); }

    function openGrant(userId, name) {
      document.getElementById('grantUserId').value = userId;
      document.getElementById('grantUserName').value = name;
      const sel = document.getElementById('grantCheatId');
      sel.innerHTML = CHEATS.map(c => `<option value="${c.id}">${esc(c.game)}  ${esc(c.name)}</option>`).join('');
      openModal('grantModal');
    }

    async function doGrant() {
      const userId = document.getElementById('grantUserId').value;
      const cheatId = document.getElementById('grantCheatId').value;
      const days = parseInt(document.getElementById('grantDays').value) || 0;
      try {
        const r = await apiFetch(`${API}/users/${userId}/grant`, { method: 'POST', body: JSON.stringify({ cheat_id: parseInt(cheatId), days: days || null }) });
        const d = await r.json();
        if (d.success) { toast('License granted!'); closeModal('grantModal'); }
        else toast(d.error, 'error');
      } catch (e) { toast('Error', 'error'); }
    }

    //  Sessions 
    async function loadSessions() {
      try {
        const r = await apiFetch(`${API}/sessions`);
        const d = await r.json();
        if (d.success) { SESSIONS = d.sessions; renderSessions(); }
      } catch (e) { console.error(e); }
    }

    function renderSessions() {
      document.getElementById('statSessions').textContent = SESSIONS.length;
      document.getElementById('sessionCount').textContent = SESSIONS.length;
      const tb = document.getElementById('sessionsTable');

      if (SESSIONS.length === 0) { tb.innerHTML = '<tr><td colspan="4"><div class="empty-state"><p>No active sessions</p></div></td></tr>'; return; }

      tb.innerHTML = SESSIONS.map(s => `<tr>
        <td><div class="user-cell"><div class="user-avatar">${(s.nickname || s.username).charAt(0).toUpperCase()}</div><div><strong>${esc(s.nickname || s.username)}</strong><br><span style="font-size:11px;color:var(--text3)">${esc(s.username)}</span></div></div></td>
        <td style="font-size:11px;color:var(--text3);max-width:180px;overflow:hidden;text-overflow:ellipsis">${s.hwid || ''}</td>
        <td style="font-size:12px;color:var(--text2)">${s.last_active ? new Date(s.last_active).toLocaleString() : ''}</td>
        <td><button class="btn btn-danger btn-sm" onclick="revokeSession(${s.id})"> Revoke</button></td>
    </tr>`).join('');
    }

    function revokeSession(id) {
      confirmAction('Revoke Session', 'This will force the user to re-login.', async () => {
        try {
          const r = await apiFetch(`${API}/sessions/revoke`, { method: 'POST', body: JSON.stringify({ session_id: id }) });
          const d = await r.json();
          if (d.success) { toast('Session revoked'); await loadSessions(); }
          else toast(d.error, 'error');
        } catch (e) { toast('Error', 'error'); }
      });
    }

    //  HWID Resets 
    async function loadResets() {
      try {
        const r = await apiFetch(`${API}/hwid_resets`);
        const d = await r.json();
        if (d.success) { RESETS = d.resets; renderResets(); }
      } catch (e) { console.error(e); }
    }

    function renderResets() {
      const pending = RESETS.filter(r => r.status === 'pending').length;
      document.getElementById('statResets').textContent = pending;
      document.getElementById('hwidCount').textContent = pending;
      const tb = document.getElementById('hwidTable');

      if (RESETS.length === 0) { tb.innerHTML = '<tr><td colspan="5"><div class="empty-state"><p>No HWID reset requests</p></div></td></tr>'; return; }

      tb.innerHTML = RESETS.map(r => `<tr>
        <td><strong>${esc(r.nickname || r.username)}</strong></td>
        <td style="max-width:200px;overflow:hidden;text-overflow:ellipsis;font-size:13px">${esc(r.reason || '')}</td>
        <td><span class="status-${r.status}">${r.status.toUpperCase()}</span></td>
        <td style="font-size:12px;color:var(--text2)">${r.created_at ? new Date(r.created_at).toLocaleString() : ''}</td>
        <td>${r.status === 'pending' ? `
            <button class="btn btn-primary btn-sm" onclick="approveReset(${r.id})" style="margin-right:4px"></button>
            <button class="btn btn-danger btn-sm" onclick="rejectReset(${r.id})"></button>
        ` : ''}</td>
    </tr>`).join('');
    }

    function approveReset(id) {
      confirmAction('Approve HWID Reset', 'This will clear the user\'s HWID and force re-login.', async () => {
        try {
          const r = await apiFetch(`${API}/hwid_reset/${id}/approve`, { method: 'POST' });
          const d = await r.json();
          if (d.success) { toast('HWID reset approved'); await loadResets(); }
          else toast(d.error, 'error');
        } catch (e) { toast('Error', 'error'); }
      });
    }

    function rejectReset(id) {
      confirmAction('Reject HWID Reset', 'Reject this reset request?', async () => {
        try {
          const r = await apiFetch(`${API}/hwid_reset/${id}/reject`, { method: 'POST' });
          const d = await r.json();
          if (d.success) { toast('HWID reset rejected'); await loadResets(); }
          else toast(d.error, 'error');
        } catch (e) { toast('Error', 'error'); }
      });
    }

    //  Helpers 
    function esc(s) { if (!s) return ''; const d = document.createElement('div'); d.textContent = s; return d.innerHTML; }

