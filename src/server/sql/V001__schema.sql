-- V001: Schema for netaccess ABAC system.
-- Tables: users, subject_attrs, sessions, resources, policies, audit_log.
-- PostgreSQL 13-17.

BEGIN;

-- 1. Users (authentication accounts)
CREATE TABLE users (
    id              BIGSERIAL PRIMARY KEY,
    username        TEXT NOT NULL UNIQUE CHECK (length(username) BETWEEN 3 AND 64),
    password_hash   TEXT NOT NULL,                -- PBKDF2-HMAC-SHA256
    salt            TEXT NOT NULL,
    full_name       TEXT NOT NULL CHECK (length(full_name) <= 200),
    position        TEXT,                          -- job title
    is_active       BOOLEAN NOT NULL DEFAULT TRUE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_login_at   TIMESTAMPTZ,
    failed_attempts INT NOT NULL DEFAULT 0,        -- lockout after 5
    locked_until    TIMESTAMPTZ                    -- lockout expiry (15 min)
);
CREATE INDEX idx_users_username ON users (username);

-- 2. Subject attributes (ABAC)
CREATE TABLE subject_attrs (
    user_id           BIGINT PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    role              TEXT NOT NULL DEFAULT 'user'    -- admin | user | auditor
                      CHECK (role IN ('admin', 'user', 'auditor')),
    clearance_level   INT  NOT NULL DEFAULT 0 CHECK (clearance_level BETWEEN 0 AND 5),
    department        TEXT                            -- department
);

-- 3. Sessions
CREATE TABLE sessions (
    id          BIGSERIAL PRIMARY KEY,
    user_id     BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash  TEXT NOT NULL UNIQUE,               -- hash of the token, not the token itself
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    expires_at  TIMESTAMPTZ NOT NULL,
    revoked_at  TIMESTAMPTZ
);
CREATE INDEX idx_sessions_user ON sessions (user_id);
CREATE INDEX idx_sessions_expires ON sessions (expires_at);

-- 4. Resources (network resource catalogue)
CREATE TABLE resources (
    id            BIGSERIAL PRIMARY KEY,
    name          TEXT NOT NULL CHECK (length(name) BETWEEN 1 AND 200),
    description   TEXT,
    resource_type TEXT NOT NULL
                  CHECK (resource_type IN ('file_share','database','printer','web_service','server','vpn')),
    address       TEXT,                                -- network address or path
    owner_id      BIGINT REFERENCES users(id) ON DELETE SET NULL,
    is_active     BOOLEAN NOT NULL DEFAULT TRUE,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX idx_resources_type ON resources (resource_type);

-- 5. Access policies (ABAC rules)
-- Conditions are typed nullable columns; integrity enforced by CHECK constraints.
CREATE TABLE policies (
    id                  BIGSERIAL PRIMARY KEY,
    name                TEXT NOT NULL UNIQUE CHECK (length(name) <= 200),
    enabled             BOOLEAN NOT NULL DEFAULT TRUE,
    action              TEXT NOT NULL
                        CHECK (action IN ('read','write','manage','connect','select','modify','print','access','admin','*')),
    role_required       TEXT CHECK (role_required IN ('admin','user','auditor')),
    department_required TEXT,
    min_clearance       INT  CHECK (min_clearance BETWEEN 0 AND 5),
    resource_type       TEXT CHECK (resource_type IN ('file_share','database','printer','web_service','server','vpn')),
    subject_id          BIGINT REFERENCES users(id) ON DELETE SET NULL,
    resource_id         BIGINT REFERENCES resources(id) ON DELETE SET NULL,
    priority            INT NOT NULL DEFAULT 0,       -- higher = more preferred
    created_by          BIGINT REFERENCES users(id) ON DELETE SET NULL,
    created_at          TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at          TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX idx_policies_enabled ON policies (enabled, priority);

-- 6. Audit log (append-only)
CREATE TABLE audit_log (
    id          BIGSERIAL PRIMARY KEY,
    ts          TIMESTAMPTZ NOT NULL DEFAULT now(),
    actor_id    BIGINT REFERENCES users(id) ON DELETE SET NULL,
    actor_name  TEXT,                                -- snapshot, not a JOIN
    action      TEXT NOT NULL,                       -- AUTH_SUCCESS, AUTH_FAILURE,
                                                      -- ACCESS_GRANTED, ACCESS_DENIED,
                                                      -- POLICY_CHANGE, RESOURCE_*, USER_*, SESSION_*
    target_type TEXT,                                -- resource | policy | user | session | system
    target_id   BIGINT,
    result      TEXT NOT NULL,                       -- ok | denied | error
    details     JSONB                               -- additional context (IP, reason, values)
);
CREATE INDEX idx_audit_ts ON audit_log (ts DESC);
CREATE INDEX idx_audit_actor ON audit_log (actor_id);
CREATE INDEX idx_audit_action ON audit_log (action);

-- Prevent UPDATE/DELETE on audit_log for all roles (append-only).
REVOKE UPDATE, DELETE ON audit_log FROM PUBLIC;

COMMIT;
