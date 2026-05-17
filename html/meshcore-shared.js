/*
 * Shared utility functions for the MeshCore web frontends.
 *
 * This file must stay free of DOM, Tabulator and Leaflet dependencies so it can
 * be reused by the desktop GUI and a future mobile GUI.
 */
(function(global)
{
    "use strict";


    const CONTACT_READ_STORAGE_KEY = "meshcore.contactLastReadEpoch";
    const PATH_DISPLAY_SETTINGS_STORAGE_KEY = "meshcore.pathDisplaySettings";
    const NOTIFICATION_AUDIO_ENABLED_STORAGE_KEY = "meshcore.notificationAudioEnabled";
    const CHANNEL_READ_STORAGE_KEY = "meshcore.channelLastReadEpoch";

    function escapeHtml(value)
    {
        return String(value)
            .replaceAll("&", "&amp;")
            .replaceAll("<", "&lt;")
            .replaceAll(">", "&gt;")
            .replaceAll('"', "&quot;")
            .replaceAll("'", "&#39;");
    }

    function isChatNode(row)
    {
        return row && row.advert_type_label === "CHAT";
    }

    function isRoomNode(row)
    {
        return row && row.advert_type_label === "ROOM";
    }

    function isChannelRow(row)
    {
        return row && row.type === "channel";
    }

    function isChatLikeNode(row)
    {
        return isChatNode(row) || isRoomNode(row) || isChannelRow(row);
    }

    function isRepeaterNode(row)
    {
        return String(row?.advert_type_label || "").toUpperCase() === "REPEATER";
    }


    function getChatKindLabel(row)
    {
        if (isRoomNode(row))
        {
            return tr("channel.type.room", "Room");
        }

        if (isChannelRow(row))
        {
            return tr("channel.type.channel", "Channel");
        }

        return tr("channel.type.chat", "Chat");
    }

    function getChatKindValue(row)
    {
        if (isRoomNode(row))
        {
            return tr("channel.kind.room", "room");
        }

        if (isChannelRow(row))
        {
            return tr("channel.kind.channel", "channel");
        }

        return tr("channel.kind.dm", "dm");
    }
    function parseMariaDbDateTime(value)
    {
        if (!value || value === "0000-00-00 00:00:00")
        {
            return 0;
        }

        return new Date(value.replace(" ", "T")).getTime();
    }

    function containsPossibleCallsign(value)
    {
        if (!value)
        {
            return false;
        }

        const text = String(value).toUpperCase();
        const regex = /(^|[^A-Z0-9])([A-Z]{1,2}[0-9][A-Z]{1,3})(?=$|[^A-Z0-9])/;

        return regex.test(text);
    }

    function getNodeLatLon(row)
    {
        if (!row)
        {
            return null;
        }

        let lat = null;
        let lon = null;

        if (row.adv_lat !== null && row.adv_lat !== undefined)
        {
            lat = Number(row.adv_lat);
        }
        else if (row.adv_lat_e6 !== null && row.adv_lat_e6 !== undefined)
        {
            lat = Number(row.adv_lat_e6) / 1000000.0;
        }

        if (row.adv_lon !== null && row.adv_lon !== undefined)
        {
            lon = Number(row.adv_lon);
        }
        else if (row.adv_lon_e6 !== null && row.adv_lon_e6 !== undefined)
        {
            lon = Number(row.adv_lon_e6) / 1000000.0;
        }

        const valid =
            Number.isFinite(lat) &&
            Number.isFinite(lon) &&
            lat !== 0 &&
            lon !== 0;

        if (!valid)
        {
            return null;
        }

        return { lat: lat, lon: lon };
    }

    function hasLocation(row)
    {
        return getNodeLatLon(row) !== null;
    }

    function hasValidCoords(node)
    {
        if (!node)
        {
            return false;
        }

        const lat = Number(node.adv_lat_e6);
        const lon = Number(node.adv_lon_e6);

        return Number.isFinite(lat) &&
            Number.isFinite(lon) &&
            lat !== 0 &&
            lon !== 0;
    }

    function hasValidEndpointCoords(endpoint)
    {
        if (!endpoint)
        {
            return false;
        }

        const lat = Number(endpoint.latitude_e6);
        const lon = Number(endpoint.longitude_e6);

        return Number.isFinite(lat) &&
            Number.isFinite(lon) &&
            lat !== 0 &&
            lon !== 0;
    }

    function e6ToDegrees(value)
    {
        return Number(value) / 1000000.0;
    }

    function degToRad(value)
    {
        return value * Math.PI / 180.0;
    }

    function distanceMeters(lat1E6, lon1E6, lat2E6, lon2E6)
    {
        const lat1 = e6ToDegrees(lat1E6);
        const lon1 = e6ToDegrees(lon1E6);
        const lat2 = e6ToDegrees(lat2E6);
        const lon2 = e6ToDegrees(lon2E6);

        const earthRadiusMeters = 6371000.0;

        const dLat = degToRad(lat2 - lat1);
        const dLon = degToRad(lon2 - lon1);

        const a =
            Math.sin(dLat / 2.0) * Math.sin(dLat / 2.0) +
            Math.cos(degToRad(lat1)) * Math.cos(degToRad(lat2)) *
            Math.sin(dLon / 2.0) * Math.sin(dLon / 2.0);

        const c = 2.0 * Math.atan2(Math.sqrt(a), Math.sqrt(1.0 - a));

        return earthRadiusMeters * c;
    }

    function formatDistanceMeters(distance)
    {
        if (!Number.isFinite(distance))
        {
            return "n/a";
        }

        if (distance < 1000.0)
        {
            return `${Math.round(distance)} m`;
        }

        return `${(distance / 1000.0).toFixed(2)} km`;
    }


const MAX_AMBIGUOUS_HOP_NEIGHBOR_DISTANCE_M = 100000.0;

function loadNotificationAudioEnabled()
{
    try
    {
        const raw = localStorage.getItem(NOTIFICATION_AUDIO_ENABLED_STORAGE_KEY);

        if (raw === null)
        {
            return true;
        }

        return raw === "1";
    }
    catch (err)
    {
        return true;
    }
}


function loadPathDisplaySettings()
{
    const defaults =
    {
        showNames: true,
        showHash: true,
        showDistances: true
    };

    try
    {
        const raw = localStorage.getItem(PATH_DISPLAY_SETTINGS_STORAGE_KEY);

        if (!raw)
        {
            return defaults;
        }

        const saved = JSON.parse(raw);

        return {
            showNames: typeof saved.showNames === "boolean" ? saved.showNames : defaults.showNames,
            showHash: typeof saved.showHash === "boolean" ? saved.showHash : defaults.showHash,
            showDistances: typeof saved.showDistances === "boolean" ? saved.showDistances : defaults.showDistances
        };
    }
    catch (err)
    {
        return defaults;
    }
}


function formatMessageText(text)
{
    const escaped = escapeHtml(String(text || ""));

    function countChar(value, ch)
    {
        let count = 0;
        for (let i = 0; i < value.length; i++)
        {
            if (value[i] === ch)
            {
                count++;
            }
        }
        return count;
    }

    function makeLink(url, original)
    {
        return '<a href="' + url + '" target="_blank" rel="noopener noreferrer">' + original + '</a>';
    }

    const withLinks = escaped.replace(
        /((https?:\/\/[^\s<]+)|(www\.[^\s<]+))/g,
        function(match)
        {
            let url = match;
            let trailing = "";

            while (url.length > 0)
            {
                const lastChar = url[url.length - 1];

                if (/[.,;:!?\]]/.test(lastChar))
                {
                    trailing = lastChar + trailing;
                    url = url.slice(0, -1);
                    continue;
                }

                if (lastChar === ")")
                {
                    const openCount = countChar(url, "(");
                    const closeCount = countChar(url, ")");

                    if (closeCount > openCount)
                    {
                        trailing = lastChar + trailing;
                        url = url.slice(0, -1);
                        continue;
                    }
                }

                break;
            }

            let href = url;

            // www. → automatisch https:// davor
            if (href.startsWith("www."))
            {
                href = "https://" + href;
            }

            return makeLink(href, url) + trailing;
        }
    );

    return withLinks.replace(/\r?\n/g, "<br>");
}


function normalizeGuiTimestamp(timestamp)
{
    if (!timestamp || timestamp <= 0)
    {
        return 0;
    }

    // Wenn Zeit in der Zukunft liegt → auf 365 Tage in der Vergangenheit setzen
    if (timestamp > Date.now())
    {
        return Date.now() - (365 * 24 * 60 * 60 * 1000);
    }

    return timestamp;
}


function getTextCharacters(text)
{
    return Array.from(String(text || ""));
}


function limitTextCharacters(text, maxLength)
{
    return getTextCharacters(text).slice(0, maxLength).join("");
}


function getLocale()
{
    const lang = getLanguage();

    switch (lang)
    {
        case "de":
            return "de-DE";
        case "en":
            return "en-GB";
        case "es":
            return "es-ES";
        case "fr":
            return "fr-FR";
        case "it":
            return "it-IT";
        default:
            return "de-DE";
    }
}


function tr(key, fallback, vars = {})
{
    if (typeof t === "function")
    {
        return t(key, vars);
    }

    let text = fallback || key;

    Object.keys(vars).forEach(function(name)
    {
        text = text.replaceAll("{" + name + "}", String(vars[name]));
    });

    return text;
}


function buildMeshCoreChannelQrPayload(channelName, secret)
{
    return `meshcore://channel/add?name=${encodeURIComponent(channelName)}&secret=${encodeURIComponent(secret)}`;
}


function formatDateTime(value)
{
    if (!value)
    {
        return "?";
    }

    const date = new Date(value.replace(" ", "T"));

    if (Number.isNaN(date.getTime()))
    {
        return value;
    }

    return date.toLocaleString(getLocale());
}


function formatEpochDateTime(epochSeconds)
{
    const value = Number(epochSeconds || 0);

    if (!Number.isFinite(value) || value <= 0)
    {
        return "-";
    }

    return new Date(value * 1000).toLocaleString(getLocale());
}


async function fetchJson(url, options = {})
{
    const response = await fetch(url, options);

    let data = null;

    try
    {
        data = await response.json();
    }
    catch (jsonError)
    {
        throw new Error(`HTTP ${response.status} (${tr("error.invalid_json", "keine gültige JSON-Antwort")})`);
    }

    if (!response.ok)
    {
        throw new Error(data?.error || `HTTP ${response.status}`);
    }

    if (!data || data.success === false)
    {
        throw new Error(data?.error || tr("error.unknown", "Unbekannter Fehler"));
    }

    return data;
}


function loadContactReadState()
{
    try
    {
        const raw = localStorage.getItem(CONTACT_READ_STORAGE_KEY);

        if (!raw)
        {
            return {};
        }

        const parsed = JSON.parse(raw);

        return parsed && typeof parsed === "object" ? parsed : {};
    }
    catch (err)
    {
        console.warn("Could not load contact read state:", err);
        return {};
    }
}


function saveContactReadState(readState)
{
    try
    {
        localStorage.setItem(CONTACT_READ_STORAGE_KEY, JSON.stringify(readState));
    }
    catch (err)
    {
        console.warn("Could not save contact read state:", err);
    }
}


function getContactReadKey(row)
{
    return `${getChatKindValue(row)}:${String(row.name || "")}`;
}


function getContactLastReadEpoch(row)
{
    const readState = loadContactReadState();
    const key = getContactReadKey(row);

    return Number(readState[key] || 0);
}


function markContactAsRead(row)
{
    const key = getContactReadKey(row);

    if (key === ":")
    {
        return;
    }

    const newestMessageEpoch = Number(row.newest_msg_epoch || 0);

    if (!Number.isFinite(newestMessageEpoch) || newestMessageEpoch <= 0)
    {
        return;
    }

    const readState = loadContactReadState();

    readState[key] = Math.max(
        Number(readState[key] || 0),
        newestMessageEpoch
    );

    saveContactReadState(readState);
}


function getOutgoingStatusClass(msg)
{
    const statusValue = Number(msg.status || 0);
    const uiState = String(msg.ui_state || "");

    if (uiState === "failed" || statusValue === 2)
    {
        return "error";
    }

    if (uiState === "confirmed" || statusValue === 1)
    {
        return "ok";
    }

    return "";
}


function extractReplyNameFromMessage(msg)
{
    const rawText = String(msg.text || msg.message_text || "").trim();

    const match = rawText.match(/^([^:\n]{1,40}):\s+/);

    if (!match)
    {
        return "";
    }

    return match[1].trim();
}


function pruneImplausibleResolvedHops(resolvedFromBack, endpoint)
{
    const resolvedForward = resolvedFromBack.slice().reverse();

    if (!Array.isArray(resolvedForward) || resolvedForward.length < 3)
    {
        return resolvedFromBack;
    }

    const endpointRef =
    {
        kind: "endpoint",
        name: endpoint.name || "endpoint",
        lat_e6: Number(endpoint.latitude_e6),
        lon_e6: Number(endpoint.longitude_e6)
    };

    for (let index = 1; index < resolvedForward.length - 1; index += 1)
    {
        const currentHop = resolvedForward[index];
        const prevHop = resolvedForward[index - 1];
        const nextHop = resolvedForward[index + 1];

        if (!currentHop || !currentHop.selected)
        {
            continue;
        }

        if (!prevHop || !prevHop.selected)
        {
            continue;
        }

        let nextRef = null;

        if (nextHop && nextHop.selected)
        {
            nextRef =
            {
                lat_e6: Number(nextHop.selected.adv_lat_e6),
                lon_e6: Number(nextHop.selected.adv_lon_e6)
            };
        }
        else if (index === resolvedForward.length - 2)
        {
            nextRef = endpointRef;
        }
        else
        {
            continue;
        }

        const distanceBeforeM = distanceMeters(
            Number(prevHop.selected.adv_lat_e6),
            Number(prevHop.selected.adv_lon_e6),
            Number(currentHop.selected.adv_lat_e6),
            Number(currentHop.selected.adv_lon_e6)
        );

        const distanceAfterM = distanceMeters(
            Number(currentHop.selected.adv_lat_e6),
            Number(currentHop.selected.adv_lon_e6),
            Number(nextRef.lat_e6),
            Number(nextRef.lon_e6)
        );

        if (
            Number.isFinite(distanceBeforeM) &&
            Number.isFinite(distanceAfterM) &&
            distanceBeforeM > MAX_AMBIGUOUS_HOP_NEIGHBOR_DISTANCE_M &&
            distanceAfterM > MAX_AMBIGUOUS_HOP_NEIGHBOR_DISTANCE_M
        )
        {
            currentHop.selected = null;
            currentHop.note = "discarded_implausible_long_before_after";
        }
    }

    return resolvedForward.reverse();
}


const MAX_ROUTE_CANDIDATES_PER_HOP = 80;
const SAME_REPEATER_PENALTY_M = 500000.0;
const ENDPOINT_ANCHOR_LAST_HOP_WEIGHT = 1.5;
const ENDPOINT_ANCHOR_SECOND_LAST_HOP_WEIGHT = 0.6;
const VERY_LARGE_ROUTE_COST = 1e18;

function normalizeRoutePoint(point, kind, nameFallback)
{
    if (!point || typeof point !== "object")
    {
        return null;
    }

    let lat = null;
    let lon = null;

    if (point.latitude_e6 !== undefined || point.longitude_e6 !== undefined)
    {
        lat = Number(point.latitude_e6);
        lon = Number(point.longitude_e6);
    }
    else
    {
        lat = Number(point.adv_lat_e6);
        lon = Number(point.adv_lon_e6);
    }

    if (!Number.isFinite(lat) || !Number.isFinite(lon))
    {
        return null;
    }

    return {
        kind: kind || "point",
        name: String(point.name || nameFallback || kind || "point"),
        prefix6_hex: String(point.prefix6_hex || ""),
        lat_e6: lat,
        lon_e6: lon
    };
}

function makeRouteCandidate(match, hopIndex)
{
    return {
        hop_index: hopIndex,
        prefix6_hex: String(match.prefix6_hex || ""),
        name: String(match.name || ""),
        adv_lat_e6: match.adv_lat_e6,
        adv_lon_e6: match.adv_lon_e6,
        has_coords: hasValidCoords(match),
        distance_m: null
    };
}

function routePointDistanceMeters(a, b)
{
    if (!a || !b)
    {
        return VERY_LARGE_ROUTE_COST;
    }

    const distance = distanceMeters(a.lat_e6, a.lon_e6, b.lat_e6, b.lon_e6);

    return Number.isFinite(distance) ? distance : VERY_LARGE_ROUTE_COST;
}

function routeCandidateDistanceMeters(a, b)
{
    if (!a || !b)
    {
        return VERY_LARGE_ROUTE_COST;
    }

    const distance = distanceMeters(
        Number(a.adv_lat_e6),
        Number(a.adv_lon_e6),
        Number(b.adv_lat_e6),
        Number(b.adv_lon_e6)
    );

    return Number.isFinite(distance) ? distance : VERY_LARGE_ROUTE_COST;
}

function routePointToCandidateDistanceMeters(point, candidate)
{
    if (!point || !candidate)
    {
        return VERY_LARGE_ROUTE_COST;
    }

    const distance = distanceMeters(
        point.lat_e6,
        point.lon_e6,
        Number(candidate.adv_lat_e6),
        Number(candidate.adv_lon_e6)
    );

    return Number.isFinite(distance) ? distance : VERY_LARGE_ROUTE_COST;
}

function routeCandidateToPointDistanceMeters(candidate, point)
{
    return routePointToCandidateDistanceMeters(point, candidate);
}

function sameRepeaterPenalty(a, b)
{
    if (!a || !b)
    {
        return 0.0;
    }

    const left = String(a.prefix6_hex || "");
    const right = String(b.prefix6_hex || "");

    if (left !== "" && left === right)
    {
        return SAME_REPEATER_PENALTY_M;
    }

    return 0.0;
}

function endpointAnchorPenaltyMeters(candidate, hopIndex, hopCount, endpointPoint)
{
    if (!candidate || !endpointPoint || hopCount <= 0)
    {
        return 0.0;
    }

    const distanceToEndpointM = routeCandidateToPointDistanceMeters(candidate, endpointPoint);

    if (!Number.isFinite(distanceToEndpointM) || distanceToEndpointM >= VERY_LARGE_ROUTE_COST)
    {
        return 0.0;
    }

    if (hopIndex === hopCount - 1)
    {
        return distanceToEndpointM * ENDPOINT_ANCHOR_LAST_HOP_WEIGHT;
    }

    if (hopIndex === hopCount - 2)
    {
        return distanceToEndpointM * ENDPOINT_ANCHOR_SECOND_LAST_HOP_WEIGHT;
    }

    return 0.0;
}

function buildRouteCandidatesForHop(hop, hopIndex, hopCount, sourcePoint, endpointPoint)
{
    const matches = Array.isArray(hop.matches) ? hop.matches : [];

    const allCandidates = matches.map(function(match)
    {
        return makeRouteCandidate(match, hopIndex);
    });

    const candidates = allCandidates.filter(function(candidate)
    {
        return candidate.has_coords;
    });

    candidates.sort(function(a, b)
    {
        const aScore = routePointToCandidateDistanceMeters(sourcePoint, a)
            + routeCandidateToPointDistanceMeters(a, endpointPoint)
            + endpointAnchorPenaltyMeters(a, hopIndex, hopCount, endpointPoint);
        const bScore = routePointToCandidateDistanceMeters(sourcePoint, b)
            + routeCandidateToPointDistanceMeters(b, endpointPoint)
            + endpointAnchorPenaltyMeters(b, hopIndex, hopCount, endpointPoint);

        return aScore - bScore;
    });

    return {
        all_candidates: allCandidates,
        candidates: candidates.slice(0, MAX_ROUTE_CANDIDATES_PER_HOP)
    };
}

function candidateAsRoutePoint(candidate)
{
    if (!candidate)
    {
        return null;
    }

    return {
        kind: "node",
        name: candidate.name || candidate.prefix6_hex || "node",
        prefix6_hex: candidate.prefix6_hex || "",
        lat_e6: Number(candidate.adv_lat_e6),
        lon_e6: Number(candidate.adv_lon_e6)
    };
}

function resolvePathBestRoute(pathEntry, endpoint, source)
{
    const sourcePoint = normalizeRoutePoint(source, "source", "source");
    const endpointPoint = normalizeRoutePoint(endpoint, "endpoint", "endpoint");

    if (!sourcePoint || !endpointPoint)
    {
        return resolvePathGreedyFromEndpoint(pathEntry, endpoint);
    }

    const hops = Array.isArray(pathEntry.hops) ? pathEntry.hops : [];

    const result =
    {
        path_id: pathEntry.id,
        created_at: pathEntry.created_at,
        path_text: pathEntry.path_text,
        hop_count: pathEntry.hop_count,
        resolved: false,
        resolved_fully: false,
        resolved_partially: false,
        resolution_mode: "unresolved",
        resolved_hops: []
    };

    if (hops.length === 0)
    {
        result.resolution_mode = "no_hops";
        return result;
    }

    const layers = hops.map(function(hop, hopIndex)
    {
        return buildRouteCandidatesForHop(hop, hopIndex, hops.length, sourcePoint, endpointPoint);
    });

    const hasMissingLayer = layers.some(function(layer)
    {
        return layer.candidates.length === 0;
    });

    if (hasMissingLayer)
    {
        const fallback = resolvePathGreedyFromEndpoint(pathEntry, endpoint);
        fallback.resolution_mode = fallback.resolved
            ? "greedy_fallback_missing_route_candidate"
            : "partial_greedy_fallback_missing_route_candidate";
        return fallback;
    }

    const dpLayers = [];

    dpLayers[0] = layers[0].candidates.map(function(candidate)
    {
        return {
            candidate: candidate,
            cost: routePointToCandidateDistanceMeters(sourcePoint, candidate)
                + endpointAnchorPenaltyMeters(candidate, 0, hops.length, endpointPoint),
            previous_index: -1
        };
    });

    for (let layerIndex = 1; layerIndex < layers.length; layerIndex += 1)
    {
        const previousLayer = dpLayers[layerIndex - 1];
        const currentCandidates = layers[layerIndex].candidates;

        dpLayers[layerIndex] = currentCandidates.map(function(candidate)
        {
            let bestCost = VERY_LARGE_ROUTE_COST;
            let bestPreviousIndex = -1;

            previousLayer.forEach(function(previousState, previousIndex)
            {
                const transitionCost = routeCandidateDistanceMeters(previousState.candidate, candidate)
                    + sameRepeaterPenalty(previousState.candidate, candidate);
                const totalCost = previousState.cost
                    + transitionCost
                    + endpointAnchorPenaltyMeters(candidate, layerIndex, hops.length, endpointPoint);

                if (totalCost < bestCost)
                {
                    bestCost = totalCost;
                    bestPreviousIndex = previousIndex;
                }
            });

            return {
                candidate: candidate,
                cost: bestCost,
                previous_index: bestPreviousIndex
            };
        });
    }

    const lastLayerIndex = dpLayers.length - 1;
    const lastLayer = dpLayers[lastLayerIndex];
    let bestLastIndex = -1;
    let bestTotalCost = VERY_LARGE_ROUTE_COST;

    lastLayer.forEach(function(state, stateIndex)
    {
        const totalCost = state.cost + routeCandidateToPointDistanceMeters(state.candidate, endpointPoint);

        if (totalCost < bestTotalCost)
        {
            bestTotalCost = totalCost;
            bestLastIndex = stateIndex;
        }
    });

    if (bestLastIndex < 0)
    {
        return resolvePathGreedyFromEndpoint(pathEntry, endpoint);
    }

    const selectedByLayer = new Array(layers.length).fill(null);
    let selectedIndex = bestLastIndex;

    for (let layerIndex = lastLayerIndex; layerIndex >= 0; layerIndex -= 1)
    {
        const state = dpLayers[layerIndex][selectedIndex];
        selectedByLayer[layerIndex] = state.candidate;
        selectedIndex = state.previous_index;
    }

    const resolvedHops = hops.map(function(hop, hopIndex)
    {
        const selected = Object.assign({}, selectedByLayer[hopIndex]);
        const nextPoint = (hopIndex + 1 < selectedByLayer.length)
            ? candidateAsRoutePoint(selectedByLayer[hopIndex + 1])
            : endpointPoint;

        selected.distance_m = routeCandidateToPointDistanceMeters(selected, nextPoint);

        return {
            hop_index: hopIndex,
            token: hop.token,
            token_len: hop.token_len,
            original_match_count: hop.match_count,
            selected: selected,
            all_candidates: layers[hopIndex].all_candidates,
            note: "selected_best_total_route",
            reference_used:
            {
                kind: nextPoint.kind,
                name: nextPoint.name,
                lat_e6: nextPoint.lat_e6,
                lon_e6: nextPoint.lon_e6
            }
        };
    });

    result.resolved = true;
    result.resolved_fully = true;
    result.resolved_partially = true;
    result.resolution_mode = "best_total_route";
    result.route_distance_m = bestTotalCost;
    result.resolved_hops = resolvedHops;

    return result;
}


function resolvePathGreedyFromEndpoint(pathEntry, endpoint)
{
    let lastSelectedPrefix6Hex = "";

    const result =
    {
        path_id: pathEntry.id,
        created_at: pathEntry.created_at,
        path_text: pathEntry.path_text,
        hop_count: pathEntry.hop_count,
        resolved: false,
        resolved_fully: false,
        resolved_partially: false,
        resolution_mode: "unresolved",
        resolved_hops: []
    };

    const hops = Array.isArray(pathEntry.hops) ? pathEntry.hops : [];

    if (!hasValidEndpointCoords(endpoint))
    {
        result.resolution_mode = "missing_endpoint_coords";
        return result;
    }

    let referencePoint =
    {
        kind: "endpoint",
        name: endpoint.name || "endpoint",
        lat_e6: Number(endpoint.latitude_e6),
        lon_e6: Number(endpoint.longitude_e6)
    };

    const resolvedFromBack = [];

    for (let hopIndex = hops.length - 1; hopIndex >= 0; hopIndex -= 1)
    {
        const hop = hops[hopIndex];
        const matches = Array.isArray(hop.matches) ? hop.matches : [];

        const hopCount = hops.length;

        const matchesWithDistance = matches.map(function(match)
        {
            const candidate =
            {
                prefix6_hex: match.prefix6_hex || "",
                name: match.name || "",
                adv_lat_e6: match.adv_lat_e6,
                adv_lon_e6: match.adv_lon_e6,
                has_coords: hasValidCoords(match),
                distance_m: null
            };

            if (candidate.has_coords)
            {
                candidate.distance_m = distanceMeters(
                    referencePoint.lat_e6,
                    referencePoint.lon_e6,
                    candidate.adv_lat_e6,
                    candidate.adv_lon_e6
                );

                candidate.anchor_penalty_m = endpointAnchorPenaltyMeters(
                    candidate,
                    hopIndex,
                    hopCount,
                    {
                        kind: "endpoint",
                        name: endpoint.name || "endpoint",
                        lat_e6: Number(endpoint.latitude_e6),
                        lon_e6: Number(endpoint.longitude_e6)
                    }
                );
            }

            return candidate;
        });

        let candidatesWithCoords = matchesWithDistance.filter(function(candidate)
        {
            return candidate.has_coords && Number.isFinite(candidate.distance_m);
        });

        const candidatesWithoutImmediateDuplicate = candidatesWithCoords.filter(function(candidate)
        {
            return (candidate.prefix6_hex || "") !== lastSelectedPrefix6Hex;
        });

        if (candidatesWithoutImmediateDuplicate.length > 0)
        {
            candidatesWithCoords = candidatesWithoutImmediateDuplicate;
        }

        if (candidatesWithCoords.length === 0)
        {
            resolvedFromBack.push(
            {
                hop_index: hopIndex,
                token: hop.token,
                token_len: hop.token_len,
                original_match_count: hop.match_count,
                selected: null,
                all_candidates: matchesWithDistance,
                note: "no_candidate_with_coords",
                reference_used:
                {
                    kind: referencePoint.kind,
                    name: referencePoint.name,
                    lat_e6: referencePoint.lat_e6,
                    lon_e6: referencePoint.lon_e6
                }
            });

            result.resolution_mode = "partial_greedy_from_endpoint";
            continue;
        }

        candidatesWithCoords.sort(function(a, b)
        {
            return (a.distance_m + (a.anchor_penalty_m || 0.0))
                - (b.distance_m + (b.anchor_penalty_m || 0.0));
        });

        const selected = candidatesWithCoords[0];

        resolvedFromBack.push(
        {
            hop_index: hopIndex,
            token: hop.token,
            token_len: hop.token_len,
            original_match_count: hop.match_count,
            selected: selected,
            all_candidates: matchesWithDistance,
            note: "selected_shortest_distance",
            reference_used:
            {
                kind: referencePoint.kind,
                name: referencePoint.name,
                lat_e6: referencePoint.lat_e6,
                lon_e6: referencePoint.lon_e6
            }
        });

        lastSelectedPrefix6Hex = selected.prefix6_hex || "";

        referencePoint =
        {
            kind: "node",
            name: selected.name,
            lat_e6: Number(selected.adv_lat_e6),
            lon_e6: Number(selected.adv_lon_e6)
        };
    }

    const prunedResolvedFromBack = pruneImplausibleResolvedHops(resolvedFromBack, endpoint);

    const unresolvedCount = prunedResolvedFromBack.filter(function(hop)
    {
        return !hop.selected;
    }).length;

    result.resolved = (unresolvedCount === 0);
    result.resolved_fully = (unresolvedCount === 0);
    result.resolved_partially = (prunedResolvedFromBack.length > 0);
    result.resolution_mode = (unresolvedCount === 0)
        ? "greedy_from_endpoint"
        : "partial_greedy_from_endpoint";
    result.resolved_hops = prunedResolvedFromBack.reverse();

    return result;
}


function buildResolvedPathListEntry(resolvedPath, endpoint)
{
    const resolvedHops = Array.isArray(resolvedPath.resolved_hops)
        ? resolvedPath.resolved_hops
        : [];

    const normalizedHops = resolvedHops.map(function(hop)
    {
        const isResolved = !!hop.selected;

        return {
            hop_index: hop.hop_index,
            token: hop.token || "",
            token_len: Number(hop.token_len || 0),
            original_match_count: Number(hop.original_match_count || 0),
            resolved: isResolved,
            note: hop.note || "",
            node: isResolved
                ? {
                    prefix6_hex: hop.selected.prefix6_hex || "",
                    name: hop.selected.name || "",
                    adv_lat_e6: hop.selected.adv_lat_e6,
                    adv_lon_e6: hop.selected.adv_lon_e6
                }
                : null,
            reference: hop.reference_used
                ? {
                    kind: hop.reference_used.kind || "",
                    name: hop.reference_used.name || "",
                    lat_e6: hop.reference_used.lat_e6,
                    lon_e6: hop.reference_used.lon_e6
                }
                : null,
            distance_m: isResolved ? hop.selected.distance_m : null
        };
    });

    return {
        path_id: resolvedPath.path_id,
        created_at: resolvedPath.created_at,
        path_text: resolvedPath.path_text,
        hop_count: Number(resolvedPath.hop_count || 0),
        resolved: !!resolvedPath.resolved,
        resolved_fully: !!resolvedPath.resolved_fully,
        resolved_partially: !!resolvedPath.resolved_partially,
        resolution_mode: resolvedPath.resolution_mode || "unresolved",
        endpoint: endpoint
            ? {
                id: endpoint.id ?? null,
                name: endpoint.name || "",
                latitude_e6: endpoint.latitude_e6 ?? null,
                longitude_e6: endpoint.longitude_e6 ?? null
            }
            : null,
        hops: normalizedHops
    };
}


function pathHasNoGaps(pathEntry)
{
    const hops = Array.isArray(pathEntry.hops) ? pathEntry.hops : [];

    if (hops.length === 0)
    {
        return false;
    }

    return hops.every(function(hop)
    {
        return !!hop.resolved;
    });
}


function selectPreferredResolvedPath(resolvedPathList)
{
    const paths = Array.isArray(resolvedPathList) ? resolvedPathList : [];

    if (paths.length === 0)
    {
        return null;
    }

    const completePaths = paths.filter(function(pathEntry)
    {
        return pathHasNoGaps(pathEntry);
    });

    const candidatePaths = completePaths.length > 0 ? completePaths : paths;

    let bestPath = candidatePaths[0];

    for (let index = 1; index < candidatePaths.length; index += 1)
    {
        const currentPath = candidatePaths[index];
        const bestHopCount = Number(bestPath.hop_count || 0);
        const currentHopCount = Number(currentPath.hop_count || 0);

        if (currentHopCount < bestHopCount)
        {
            bestPath = currentPath;
        }
    }

    return bestPath;
}


function buildResolvedPathList(paths, endpoint, source = null)
{
    return paths.map(function(pathEntry)
    {
        const resolvedPath = source
            ? resolvePathBestRoute(pathEntry, endpoint, source)
            : resolvePathGreedyFromEndpoint(pathEntry, endpoint);
        return buildResolvedPathListEntry(resolvedPath, endpoint);
    });
}


function loadChannelReadState()
{
    try
    {
        const raw = localStorage.getItem(CHANNEL_READ_STORAGE_KEY);

        if (!raw)
        {
            return {};
        }

        const parsed = JSON.parse(raw);

        return parsed && typeof parsed === "object" ? parsed : {};
    }
    catch (err)
    {
        console.warn("Could not load channel read state:", err);
        return {};
    }
}


function saveChannelReadState(readState)
{
    try
    {
        localStorage.setItem(CHANNEL_READ_STORAGE_KEY, JSON.stringify(readState));
    }
    catch (err)
    {
        console.warn("Could not save channel read state:", err);
    }
}


function getChannelReadKey(channel)
{
    return String(channel.key_hex || "").toUpperCase();
}


function getChannelLastReadEpoch(channel)
{
    const readState = loadChannelReadState();
    const key = getChannelReadKey(channel);

    return Number(readState[key] || 0);
}


function markChannelAsRead(channel)
{
    const key = getChannelReadKey(channel);

    if (key === "")
    {
        return;
    }

    const newestMessageEpoch = Number(channel.newest_message_epoch || 0);

    if (!Number.isFinite(newestMessageEpoch) || newestMessageEpoch <= 0)
    {
        return;
    }

    const readState = loadChannelReadState();

    readState[key] = Math.max(
        Number(readState[key] || 0),
        newestMessageEpoch
    );

    saveChannelReadState(readState);
}



    global.MeshCoreShared =
    {
        escapeHtml,
        isChatNode,
        isRoomNode,
        isChannelRow,
        isChatLikeNode,
        isRepeaterNode,
        getChatKindLabel,
        getChatKindValue,
        parseMariaDbDateTime,
        containsPossibleCallsign,
        getNodeLatLon,
        hasLocation,
        hasValidCoords,
        hasValidEndpointCoords,
        e6ToDegrees,
        degToRad,
        distanceMeters,
        formatDistanceMeters,
        loadNotificationAudioEnabled,
        loadPathDisplaySettings,
        formatMessageText,
        normalizeGuiTimestamp,
        getTextCharacters,
        limitTextCharacters,
        getLocale,
        tr,
        buildMeshCoreChannelQrPayload,
        formatDateTime,
        formatEpochDateTime,
        fetchJson,
        loadContactReadState,
        saveContactReadState,
        getContactReadKey,
        getContactLastReadEpoch,
        markContactAsRead,
        getOutgoingStatusClass,
        extractReplyNameFromMessage,
        pruneImplausibleResolvedHops,
        resolvePathGreedyFromEndpoint,
        resolvePathBestRoute,
        buildResolvedPathListEntry,
        pathHasNoGaps,
        selectPreferredResolvedPath,
        buildResolvedPathList,
        loadChannelReadState,
        saveChannelReadState,
        getChannelReadKey,
        getChannelLastReadEpoch,
        markChannelAsRead
    };

    if (!global.MeshCoreApi)
    {
        function jsonHeaders(extraHeaders = {})
        {
            return Object.assign(
            {
                "Accept": "application/json"
            }, extraHeaders);
        }

        function noStoreGet()
        {
            return {
                method: "GET",
                cache: "no-store",
                headers: jsonHeaders()
            };
        }

        function postJson(payload)
        {
            return {
                method: "POST",
                headers: jsonHeaders(
                {
                    "Content-Type": "application/json"
                }),
                body: JSON.stringify(payload || {})
            };
        }

        function postEmpty()
        {
            return {
                method: "POST",
                headers: jsonHeaders()
            };
        }

        function withCacheBuster(url)
        {
            const separator = url.indexOf("?") === -1 ? "?" : "&";
            return `${url}${separator}_=${Date.now()}`;
        }

        global.MeshCoreApi =
        {
            async loadNodes(type = "all")
            {
                return await fetchJson(withCacheBuster(`nodes.php?type=${encodeURIComponent(type)}`), noStoreGet());
            },

            async loadCompanionRadioStatus()
            {
                return await fetchJson("companion_radio_status.php", noStoreGet());
            },

            async startDiscover()
            {
                return await fetchJson("discover_start.php", postEmpty());
            },

            async loadDiscoverStatus()
            {
                return await fetchJson("discover_status.php", noStoreGet());
            },

            async clearDiscoverRequest()
            {
                return await fetchJson("discover_clear.php", postEmpty());
            },

            async loadNodePath(nodeId)
            {
                return await fetchJson(withCacheBuster(`node_path.php?node_id=${encodeURIComponent(nodeId)}`), noStoreGet());
            },

            async loadMessagePath(correlationKey)
            {
                return await fetchJson(withCacheBuster(`message_path.php?correlation_key=${encodeURIComponent(correlationKey)}`), noStoreGet());
            },

            async loadMessages(params)
            {
                const chatKind = params && params.kind ? String(params.kind) : "direct";

                if (chatKind === "channel")
                {
                    return await fetchJson(withCacheBuster(`messages.php?kind=channel&channel_key_hex=${encodeURIComponent(String(params.channel_key_hex || ""))}`), noStoreGet());
                }

                return await fetchJson(withCacheBuster(`messages.php?kind=${encodeURIComponent(chatKind)}&name=${encodeURIComponent(String(params.name || ""))}`), noStoreGet());
            },

            async loadNewMessages(afterId)
            {
                return await fetchJson(`new_messages.php?after_id=${encodeURIComponent(afterId)}`, noStoreGet());
            },

            async saveRoomPassword(context, password)
            {
                return await fetchJson("save_room_password.php", postJson(
                {
                    room_node_id: context.roomNodeId,
                    room_name: context.roomName,
                    password: password
                }));
            },

            async loadTxStatus(txId)
            {
                return await fetchJson(`tx_status.php?id=${encodeURIComponent(txId)}`, noStoreGet());
            },

            async sendMessage(payload)
            {
                return await fetchJson("send_message.php", postJson(payload));
            },

            async sendFloodAdvert()
            {
                return await this.sendMessage(
                {
                    tx_kind: 2,
                    message_text: "[flood advert]",
                    max_retries: 1
                });
            },

            async loadChannels()
            {
                return await fetchJson("channels.php", noStoreGet());
            },

            async saveChannel(payload)
            {
                return await fetchJson("save_channel.php", postJson(payload));
            },

            async deleteChannel(keyHex)
            {
                return await fetchJson("delete_channel.php", postJson(
                {
                    key_hex: String(keyHex || "")
                }));
            },

            async loadCompanionSetup()
            {
                return await fetchJson("companion_setup_read.php", noStoreGet());
            },

            async applyCompanionSetup(payload)
            {
                return await fetchJson("companion_setup.php", postJson(payload));
            },

            async resetNodePath(publicKeyHex)
            {
                return await fetchJson("reset_node_path.php", postJson(
                {
                    public_key_hex: String(publicKeyHex || "")
                }));
            }
        };
    }

})(window);
