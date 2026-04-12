<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

function parsePathTokens(?string $pathText): array
{
    if ($pathText === null)
    {
        return [];
    }

    $parts = explode(',', $pathText);
    $tokens = [];

    foreach ($parts as $part)
    {
        $token = strtolower(trim($part));

        if ($token === '')
        {
            continue;
        }

        $tokens[] = $token;
    }

    return $tokens;
}

function resolvePathToken(mysqli_stmt $stmt, string $token): array
{
    $matches = [];

    if (!preg_match('/^(?:[0-9a-f]{2}|[0-9a-f]{4}|[0-9a-f]{6})$/', $token))
    {
        return $matches;
    }

    $likeValue = $token . '%';

    $stmt->bind_param('s', $likeValue);
    $stmt->execute();
    $stmt->bind_result($prefix6Hex, $name, $advLatE6, $advLonE6);

    while ($stmt->fetch())
    {
        $matches[] =
        [
            'prefix6_hex' => ($prefix6Hex !== null) ? (string) $prefix6Hex : null,
            'name'        => (string) $name,
            'adv_lat_e6'  => ($advLatE6 !== null) ? (int) $advLatE6 : null,
            'adv_lon_e6'  => ($advLonE6 !== null) ? (int) $advLonE6 : null,
        ];
    }

    $stmt->free_result();
    $stmt->reset();

    return $matches;
}

function readCompanionEndpoint(mysqli $db): array
{
    $sql = "
        SELECT
            id,
            name,
            latitude_e6,
            longitude_e6
        FROM companion_config
        ORDER BY id ASC
        LIMIT 1
    ";

    $stmt = $db->prepare($sql);
    $stmt->execute();
    $stmt->bind_result($id, $name, $latitudeE6, $longitudeE6);

    $endpoint =
    [
        'id'           => null,
        'name'         => null,
        'latitude_e6'  => null,
        'longitude_e6' => null,
    ];

    if ($stmt->fetch())
    {
        $endpoint =
        [
            'id'           => (int) $id,
            'name'         => (string) $name,
            'latitude_e6'  => (int) $latitudeE6,
            'longitude_e6' => (int) $longitudeE6,
        ];
    }

    $stmt->close();

    return $endpoint;
}

try
{
    $correlationKey = trim((string) ($_GET['correlation_key'] ?? ''));

    if ($correlationKey === '')
    {
        throw new RuntimeException('Kein correlation_key angegeben.');
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

    $sqlMessages = "
        SELECT
            id,
            created_at,
            path_text
        FROM rx_log_messages
        WHERE correlation_key = ?
        ORDER BY created_at ASC, id ASC
    ";

    $stmtMessages = $db->prepare($sqlMessages);
    $stmtMessages->bind_param('s', $correlationKey);
    $stmtMessages->execute();
    $stmtMessages->bind_result($id, $createdAt, $pathText);

    $messageRows = [];

    while ($stmtMessages->fetch())
    {
        $messageRows[] =
        [
            'id'         => (int) $id,
            'created_at' => (string) $createdAt,
            'path_text'  => ($pathText !== null) ? (string) $pathText : null,
        ];
    }

    $stmtMessages->close();

    $sqlNodes = "
        SELECT
            prefix6_hex,
            name,
            adv_lat_e6,
            adv_lon_e6
        FROM nodes
        WHERE prefix6_hex IS NOT NULL
          AND LOWER(prefix6_hex) LIKE ?
        ORDER BY name ASC, prefix6_hex ASC
    ";

    $stmtNodes = $db->prepare($sqlNodes);

    $rows = [];

    foreach ($messageRows as $messageRow)
    {
        $pathTextValue = $messageRow['path_text'];
        $tokens = parsePathTokens($pathTextValue);
        $hops = [];

        foreach ($tokens as $token)
        {
            $matches = resolvePathToken($stmtNodes, $token);

            $hops[] =
            [
                'token'       => $token,
                'token_len'   => strlen($token),
                'match_count' => count($matches),
                'matches'     => $matches,
            ];
        }

        $rows[] =
        [
            'id'         => $messageRow['id'],
            'created_at' => $messageRow['created_at'],
            'path_text'  => $pathTextValue,
            'hop_count'  => count($hops),
            'hops'       => $hops,
        ];
    }

        $stmtNodes->close();

    $endpoint = readCompanionEndpoint($db);

    $db->close();

    echo json_encode(
        [
            'success'         => true,
            'correlation_key' => $correlationKey,
            'endpoint'        => $endpoint,
            'path_count'      => count($rows),
            'paths'           => $rows,
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT
    );
}
catch (Throwable $e)
{
    http_response_code(400);

    echo json_encode(
        [
            'success' => false,
            'error'   => $e->getMessage(),
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT
    );
}