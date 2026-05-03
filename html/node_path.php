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
    $nodeDbId = (int) ($_GET['node_id'] ?? 0);

    if ($nodeDbId <= 0)
    {
        throw new RuntimeException('Keine gültige node_id angegeben.');
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

    $sqlNode = "
        SELECT
            n.id,
            n.name,
            n.adv_lat_e6,
            n.adv_lon_e6,
            p.path_text,
            p.path_len,
            p.path_hash_size,
            p.last_seen_at
        FROM nodes n
        LEFT JOIN node_advert_paths p
            ON LOWER(p.public_key_hex) = LOWER(n.public_key_hex)
        WHERE n.id = ?
        LIMIT 1
    ";

    $stmtNode = $db->prepare($sqlNode);
    $stmtNode->bind_param('i', $nodeDbId);
    $stmtNode->execute();
    $stmtNode->bind_result(
        $id,
        $name,
        $advLatE6,
        $advLonE6,
        $pathText,
        $pathLen,
        $pathHashSize,
        $pathAt
    );

    if (!$stmtNode->fetch())
    {
        throw new RuntimeException('Node nicht gefunden.');
    }

    $node =
    [
        'id' => (int) $id,
        'name' => (string) $name,
        'adv_lat_e6' => ($advLatE6 !== null) ? (int) $advLatE6 : null,
        'adv_lon_e6' => ($advLonE6 !== null) ? (int) $advLonE6 : null,
    ];

    $pathTextValue = ($pathText !== null) ? (string) $pathText : null;
    $pathAtValue = ($pathAt !== null) ? (string) $pathAt : null;

    $stmtNode->close();

    /* nur Repeater ! */
    $sqlNodes = "
        SELECT
            prefix6_hex,
            name,
            adv_lat_e6,
            adv_lon_e6
        FROM nodes
        WHERE prefix6_hex IS NOT NULL
        AND LOWER(prefix6_hex) LIKE ?
        AND advert_type = 2
        ORDER BY name ASC, prefix6_hex ASC
    ";

    $stmtNodes = $db->prepare($sqlNodes);

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

    $stmtNodes->close();

    $endpoint = readCompanionEndpoint($db);

    $db->close();

    echo json_encode(
        [
            'success' => true,
            'node' => $node,
            'endpoint' => $endpoint,
            'path' =>
            [
                'id' => (int) $nodeDbId,
                'created_at' => $pathAtValue,
                'path_text' => $pathTextValue,
                'path_len' => ($pathLen !== null) ? (int) $pathLen : null,
                'path_hash_size' => ($pathHashSize !== null) ? (int) $pathHashSize : null,
                'hop_count' => count($hops),
                'hops' => $hops,
            ],
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
            'error' => $e->getMessage(),
        ],
        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT
    );
}