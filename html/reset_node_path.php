<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

function jsonResponse(array $data, int $statusCode = 200): never
{
    http_response_code($statusCode);
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

try
{
    $raw = file_get_contents('php://input');

    if ($raw === false || $raw === '')
    {
        throw new RuntimeException('Keine Eingabedaten erhalten.');
    }

    $data = json_decode($raw, true, 512, JSON_THROW_ON_ERROR);

    $publicKeyHex = strtoupper(trim((string) ($data['public_key_hex'] ?? '')));

    if (!preg_match('/^[0-9A-F]{64}$/', $publicKeyHex))
    {
        throw new RuntimeException('Ungültige public_key_hex.');
    }

    $db = new mysqli(
        'localhost',
        'meshcore',
        '',
        'meshcore',
        3306,
        '/run/mysqld/mysqld.sock'
    );

    $db->set_charset('utf8mb4');

    $check = $db->prepare(
        "SELECT id
         FROM companion_actions
         WHERE action_type = 'reset_path'
           AND public_key_hex = ?
           AND status IN (0, 1)
         LIMIT 1"
    );
    $check->bind_param('s', $publicKeyHex);
    $check->execute();
    $result = $check->get_result();

    if ($result && $result->fetch_assoc())
    {
        jsonResponse(
            [
                'success' => true,
                'queued' => false,
                'message' => 'Für diesen Node ist bereits ein Reset vorgemerkt.'
            ]
        );
    }

    $stmt = $db->prepare(
        "INSERT INTO companion_actions
         (
             action_type,
             public_key_hex,
             status
         )
         VALUES
         (
             'reset_path',
             ?,
             0
         )"
    );

    $stmt->bind_param('s', $publicKeyHex);
    $stmt->execute();

    jsonResponse(
        [
            'success' => true,
            'queued' => true,
            'id' => (int) $db->insert_id
        ]
    );
}
catch (Throwable $e)
{
    jsonResponse(
        [
            'success' => false,
            'error' => $e->getMessage()
        ],
        500
    );
}