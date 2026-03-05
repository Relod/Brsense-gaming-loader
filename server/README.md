# Server Backend (BR Sense)

## Stack

- Node.js + Express
- MariaDB (`mysql2/promise`)
- JWT para autenticação
- `multer` para upload de payload

## Módulos

```text
src/
  config.js
  database.js
  middleware.js
  schema.js
  controllers/
    auth-controller.js
  middleware/
    admin-jwt.js
  services/
    license-service.js
    hwid-reset-service.js
    payload-service.js
  routes/
    auth.js
    status.js
    download.js
    admin.js
    admin/
      auth.js
      cheats.js
      users.js
      sessions.js
      hwid-resets.js
      panel.js
```

## Responsabilidades

- `routes/*.js`: mapeamento HTTP
- `controllers/*`: coordenação de fluxo de requisição
- `services/*`: regras reutilizáveis e acesso de suporte
- `middleware/*`: validação/autorização transversal

## Endpoints

### Público/API do loader
- `POST /api/login`
- `GET /api/cheats`
- `POST /api/heartbeat`
- `POST /api/refresh`
- `POST /api/hwid_reset`
- `POST /api/hwid_reset_by_creds`
- `GET /api/status`
- `GET /api/download/:cheat_id`

### Admin API
- `POST /api/admin/login`
- `GET /api/admin/cheats`
- `POST /api/admin/cheats`
- `PUT /api/admin/cheats/:id`
- `DELETE /api/admin/cheats/:id`
- `GET /api/admin/users`
- `POST /api/admin/users/:userId/grant`
- `DELETE /api/admin/users/:userId/revoke/:cheatId`
- `GET /api/admin/sessions`
- `POST /api/admin/sessions/revoke`
- `GET /api/admin/hwid_resets`
- `POST /api/admin/hwid_reset/:id/approve`
- `POST /api/admin/hwid_reset/:id/reject`

### Painel
- `GET /admin`

## Setup

1. Criar banco e tabelas com `setup.sql`
2. Criar `.env` baseado em `.env.example`
3. `npm install`
4. `npm start`

## Verificação de sintaxe

```bash
node --check index.js
node --check src/routes/auth.js
node --check src/routes/admin.js
```
