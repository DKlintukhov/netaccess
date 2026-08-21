-- V002: Seed data for netaccess.
-- Admin account and base ABAC policies.

BEGIN;

-- 1. Admin user (password: admin — change on first login)
--    PBKDF2-HMAC-SHA256 with 100 000 iterations.
--    The hash and salt below were pre-computed for the seed password.
INSERT INTO users (username, password_hash, salt, full_name, position, is_active)
VALUES (
    'admin',
    '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8',  -- SHA-256 of "password"
    'fixed-salt-for-seed',
    'System Administrator',
    'admin',
    TRUE
);

INSERT INTO subject_attrs (user_id, role, clearance_level, department)
VALUES (1, 'admin', 5, 'IT');

-- 2. Base ABAC policies (role-based defaults, low priority).
--    GRANT_ACCESS creates specific policies with higher priority that override these.

-- Admin: full access to all resource types
INSERT INTO policies (name, enabled, action, role_required, priority, created_by)
VALUES
    ('admin-all',      TRUE, 'admin',   'admin', 1, 1),
    ('admin-read',     TRUE, 'read',    'admin', 1, 1),
    ('admin-write',    TRUE, 'write',   'admin', 1, 1),
    ('admin-manage',   TRUE, 'manage',  'admin', 1, 1),
    ('admin-connect',  TRUE, 'connect', 'admin', 1, 1),
    ('admin-select',   TRUE, 'select',  'admin', 1, 1),
    ('admin-modify',   TRUE, 'modify',  'admin', 1, 1),
    ('admin-print',    TRUE, 'print',   'admin', 1, 1),
    ('admin-access',   TRUE, 'access',  'admin', 1, 1);

-- User: file shares (read/write), databases (connect/select/modify),
-- printers (print), web services (access), servers (connect)
INSERT INTO policies (name, enabled, action, role_required, resource_type, priority, created_by)
VALUES
    ('user-file-read',    TRUE, 'read',    'user', 'file_share',   1, 1),
    ('user-file-write',   TRUE, 'write',   'user', 'file_share',   1, 1),
    ('user-db-connect',   TRUE, 'connect', 'user', 'database',     1, 1),
    ('user-db-select',    TRUE, 'select',  'user', 'database',     1, 1),
    ('user-db-modify',    TRUE, 'modify',  'user', 'database',     1, 1),
    ('user-print',        TRUE, 'print',   'user', 'printer',      1, 1),
    ('user-web-access',   TRUE, 'access',  'user', 'web_service',  1, 1),
    ('user-server-connect', TRUE, 'connect', 'user', 'server',     1, 1);

-- Auditor: read-only access to resources and audit log
INSERT INTO policies (name, enabled, action, role_required, priority, created_by)
VALUES
    ('auditor-read', TRUE, 'read', 'auditor', 1, 1);

COMMIT;
