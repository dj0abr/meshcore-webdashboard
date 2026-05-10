(function(global)
{
    "use strict";

    const Shared = global.MeshCoreShared || {};
    const Api = global.MeshCoreApi || {};

    const escapeHtml = Shared.escapeHtml || function(value)
    {
        return String(value)
            .replaceAll("&", "&amp;")
            .replaceAll("<", "&lt;")
            .replaceAll(">", "&gt;")
            .replaceAll('"', "&quot;")
            .replaceAll("'", "&#39;");
    };

    const state =
    {
        view: "contacts",
        nodes: [],
        channels: [],
        currentChat: null,
        currentMapRow: null,
        currentMessages: [],
        leafletMap: null,
        leafletLayer: null,
        currentMapMode: "position",
        currentMapPreferredPath: null,
        pathLabelsVisible: true,
        mapReturnView: "contacts",
        contactFilters:
        {
            type: "all",
            activeOnly: false,
            localOnly: true,
            sort: "time_desc"
        },
        refreshTimer: null,
        discoverPollTimer: null,
        discoverPending: false,
        discoverPendingJobId: null,
        discoverResults: [],
        radioStatusRefreshTimer: null,
        channelDialog: null,
        busy: false
    };

    const el =
    {
        view: document.getElementById("mobileView"),
        title: document.getElementById("mobileTitle"),
        subtitle: document.getElementById("mobileSubtitle"),
        backButton: document.getElementById("mobileBackButton"),
        refreshButton: document.getElementById("mobileRefreshButton"),
        composer: document.getElementById("mobileComposer"),
        input: document.getElementById("mobileMessageInput"),
        sendButton: document.getElementById("mobileSendButton"),
        nav: document.getElementById("mobileNav"),
        setupModal: document.getElementById("setupModal"),
        setupNameInput: document.getElementById("setupNameInput"),
        setupCityInput: document.getElementById("setupCityInput"),
        setupLatInput: document.getElementById("setupLatInput"),
        setupLonInput: document.getElementById("setupLonInput"),
        setupApplyButton: document.getElementById("setupApplyButton"),
        setupCancelButton: document.getElementById("setupCancelButton"),
        setupModalError: document.getElementById("setupModalError"),
        discoverModal: document.getElementById("discoverModal"),
        discoverStatusText: document.getElementById("discoverStatusText"),
        discoverJobInfo: document.getElementById("discoverJobInfo"),
        discoverResults: document.getElementById("discoverResults"),
        discoverRepeatButton: document.getElementById("discoverRepeatButton"),
        discoverCloseButton: document.getElementById("discoverCloseButton"),
        discoverModalError: document.getElementById("discoverModalError"),
        languageModal: document.getElementById("languageModal"),
        languageSelect: document.getElementById("languageSelect"),
        languageCloseButton: document.getElementById("languageCloseButton"),
        channelModal: document.getElementById("channelModal"),
        channelModalTitle: document.getElementById("channelModalTitle"),
        channelModalSubtitle: document.getElementById("channelModalSubtitle"),
        channelNameGroup: document.getElementById("channelNameGroup"),
        channelNameInput: document.getElementById("channelNameInput"),
        channelSecretGroup: document.getElementById("channelSecretGroup"),
        channelSecretInput: document.getElementById("channelSecretInput"),
        channelRemoveGroup: document.getElementById("channelRemoveGroup"),
        channelRemoveSelect: document.getElementById("channelRemoveSelect"),
        channelResultGroup: document.getElementById("channelResultGroup"),
        channelResultSecret: document.getElementById("channelResultSecret"),
        channelQrCode: document.getElementById("channelQrCode"),
        channelModalError: document.getElementById("channelModalError"),
        channelConfirmButton: document.getElementById("channelConfirmButton"),
        channelCancelButton: document.getElementById("channelCancelButton")
    };

    function tr(key, fallback, vars)
    {
        if (typeof Shared.tr === "function")
        {
            return Shared.tr(key, fallback, vars);
        }

        return fallback || key;
    }

    function getChatKindValue(row)
    {
        if (typeof Shared.getChatKindValue === "function")
        {
            return Shared.getChatKindValue(row);
        }

        if (row && row.type === "channel")
        {
            return "channel";
        }

        if (row && String(row.advert_type_label || "").toUpperCase() === "ROOM")
        {
            return "room";
        }

        return "dm";
    }

    function getChatKindLabel(row)
    {
        if (typeof Shared.getChatKindLabel === "function")
        {
            return Shared.getChatKindLabel(row);
        }

        return getChatKindValue(row).toUpperCase();
    }

    function isChatLikeNode(row)
    {
        if (typeof Shared.isChatLikeNode === "function")
        {
            return Shared.isChatLikeNode(row);
        }

        const type = String(row && row.advert_type_label || "").toUpperCase();
        return type === "CHAT" || type === "ROOM" || row?.type === "channel";
    }

    function isRoomNode(row)
    {
        if (typeof Shared.isRoomNode === "function")
        {
            return Shared.isRoomNode(row);
        }

        return String(row && row.advert_type_label || "").toUpperCase() === "ROOM";
    }

    function isNodeInfoOnly(row)
    {
        const typeKey = getNodeTypeKey(row);
        return typeKey === "repeater" || typeKey === "sensor";
    }

    function formatNodeInfoValue(value, fallback)
    {
        if (value === null || value === undefined || value === "")
        {
            return fallback || "-";
        }

        return String(value);
    }

    function formatNodeHex(value)
    {
        const text = String(value || "").trim();

        if (text === "")
        {
            return "-";
        }

        if (text.length <= 18)
        {
            return text;
        }

        return `${text.slice(0, 8)}…${text.slice(-8)}`;
    }

    function isChannelRow(row)
    {
        if (typeof Shared.isChannelRow === "function")
        {
            return Shared.isChannelRow(row);
        }

        return row && row.type === "channel";
    }

    function normalizeGuiEpochSeconds(epochSeconds)
    {
        const epoch = Number(epochSeconds || 0);

        if (!Number.isFinite(epoch) || epoch <= 0)
        {
            return 0;
        }

        if (typeof Shared.normalizeGuiTimestamp === "function")
        {
            return Math.floor(Shared.normalizeGuiTimestamp(epoch * 1000) / 1000);
        }

        if (epoch * 1000 > Date.now())
        {
            return Math.floor((Date.now() - (365 * 24 * 60 * 60 * 1000)) / 1000);
        }

        return epoch;
    }

    function parseGuiDateTimeEpochSeconds(value)
    {
        if (!value)
        {
            return 0;
        }

        if (typeof Shared.parseMariaDbDateTime === "function")
        {
            return normalizeGuiEpochSeconds(Math.floor(Shared.parseMariaDbDateTime(value) / 1000));
        }

        const parsed = Date.parse(String(value).replace(" ", "T"));

        if (Number.isNaN(parsed))
        {
            return 0;
        }

        return normalizeGuiEpochSeconds(Math.floor(parsed / 1000));
    }

    function formatNormalizedGuiDateTime(value, epoch)
    {
        const normalizedEpoch = normalizeGuiEpochSeconds(epoch || 0) || parseGuiDateTimeEpochSeconds(value);

        if (normalizedEpoch > 0)
        {
            if (typeof Shared.formatEpochDateTime === "function")
            {
                return Shared.formatEpochDateTime(normalizedEpoch);
            }

            return new Date(normalizedEpoch * 1000).toLocaleString();
        }

        if (typeof Shared.formatDateTime === "function")
        {
            return Shared.formatDateTime(value);
        }

        return String(value || "");
    }

    function formatDate(value, epoch)
    {
        if (Number(epoch || 0) > 0 && typeof Shared.formatEpochDateTime === "function")
        {
            return Shared.formatEpochDateTime(epoch);
        }

        if (typeof Shared.formatDateTime === "function")
        {
            return Shared.formatDateTime(value);
        }

        return String(value || "");
    }

    function formatMessageText(value)
    {
        if (typeof Shared.formatMessageText === "function")
        {
            return Shared.formatMessageText(value);
        }

        return escapeHtml(value || "").replaceAll("\n", "<br>");
    }

    function setBusy(busy)
    {
        state.busy = busy;
        el.refreshButton.disabled = busy;
        el.sendButton.disabled = busy;
    }

    function setTitle(title, subtitle)
    {
        el.title.textContent = title;
        el.subtitle.textContent = subtitle || "";
    }

    function showError(message)
    {
        el.view.innerHTML = `<div class="mobile-error">${escapeHtml(message)}</div>`;
    }

    function setNavActive(view)
    {
        for (const button of el.nav.querySelectorAll(".nav-button"))
        {
            button.classList.toggle("active", button.dataset.view === view);
        }
    }

    function stopRefreshTimer()
    {
        if (state.refreshTimer !== null)
        {
            clearInterval(state.refreshTimer);
            state.refreshTimer = null;
        }
    }

    function startRefreshTimer()
    {
        stopRefreshTimer();

        if (!state.currentChat)
        {
            return;
        }

        state.refreshTimer = setInterval(function()
        {
            loadChatMessages(state.currentChat, true).catch(function(error)
            {
                console.warn("Mobile chat refresh failed:", error);
            });
        }, 2500);
    }

    function showComposer(show)
    {
        el.composer.hidden = !show;
    }

    function showBack(show)
    {
        el.backButton.hidden = !show;
    }

    function showNav(show)
    {
        el.nav.hidden = !show;
    }

    function destroyMobileMap()
    {
        if (state.leafletMap)
        {
            state.leafletMap.remove();
            state.leafletMap = null;
            state.leafletLayer = null;
            state.currentMapMode = "position";
            state.currentMapPreferredPath = null;
        }
    }

    function buildOutgoingPayload(row, messageText)
    {
        if (isRoomNode(row))
        {
            return {
                tx_kind: 1,
                room_name: row.name || "",
                room_node_id: Number(row.node_id || 0),
                message_text: messageText,
                max_retries: 3
            };
        }

        if (isChannelRow(row))
        {
            return {
                tx_kind: 3,
                channel_name: row.name || "",
                channel_key_hex: String(row.key_hex || ""),
                message_text: messageText,
                max_retries: 3
            };
        }

        return {
            tx_kind: 0,
            target_name: row.name || "",
            target_node_id: Number(row.node_id || 0),
            message_text: messageText,
            max_retries: 3
        };
    }


    function getNodeTypeKey(row)
    {
        const type = String(row && row.advert_type_label || "").trim().toUpperCase();

        if (type === "CHAT")
        {
            return "chat";
        }

        if (type === "REPEATER")
        {
            return "repeater";
        }

        if (type === "ROOM")
        {
            return "room";
        }

        if (type === "SENSOR")
        {
            return "sensor";
        }

        return "other";
    }

    function getNodeTypeLabel(row)
    {
        const typeKey = getNodeTypeKey(row);

        if (typeKey === "chat")
        {
            return "Chat";
        }

        if (typeKey === "repeater")
        {
            return "Repeater";
        }

        if (typeKey === "room")
        {
            return "Room";
        }

        if (typeKey === "sensor")
        {
            return "Sensor";
        }

        return String(row && row.advert_type_label || "Node");
    }



    function getNodePosition(row)
    {
        if (typeof Shared.getNodeLatLon === "function")
        {
            return Shared.getNodeLatLon(row);
        }

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

        if (!Number.isFinite(lat) || !Number.isFinite(lon) || lat === 0 || lon === 0)
        {
            return null;
        }

        return { lat: lat, lon: lon };
    }

    function createLeafletIcon(row)
    {
        if (!global.L)
        {
            return undefined;
        }

        const typeKey = getNodeTypeKey(row);
        let iconUrl = "marker-icon-green.png";

        if (typeKey === "repeater")
        {
            iconUrl = "marker-icon-blue.png";
        }
        else if (typeKey === "room")
        {
            iconUrl = "marker-icon-violet.png";
        }

        return L.icon(
        {
            iconUrl: iconUrl,
            iconSize: [25, 41],
            iconAnchor: [12, 41],
            popupAnchor: [1, -34],
            shadowUrl: "https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png",
            shadowSize: [41, 41]
        });
    }


    function hasValidPathNodeCoords(node)
    {
        if (typeof Shared.hasValidCoords === "function")
        {
            return Shared.hasValidCoords(node);
        }

        const lat = Number(node && node.adv_lat_e6);
        const lon = Number(node && node.adv_lon_e6);

        return Number.isFinite(lat) && Number.isFinite(lon) && lat !== 0 && lon !== 0;
    }

    function hasValidPathEndpointCoords(endpoint)
    {
        if (typeof Shared.hasValidEndpointCoords === "function")
        {
            return Shared.hasValidEndpointCoords(endpoint);
        }

        const lat = Number(endpoint && endpoint.latitude_e6);
        const lon = Number(endpoint && endpoint.longitude_e6);

        return Number.isFinite(lat) && Number.isFinite(lon) && lat !== 0 && lon !== 0;
    }

    function getPathPointDisplayName(point)
    {
        if (!point)
        {
            return "-";
        }

        const name = String(point.name || "").trim();
        const hash = String(point.prefix6_hex || point.token || "").trim();

        if (name !== "")
        {
            return name;
        }

        if (hash !== "")
        {
            return hash;
        }

        if (point.type === "endpoint")
        {
            return "Endpoint";
        }

        return "Hop";
    }

    function getPathPointTooltipText(point)
    {
        if (!point)
        {
            return "";
        }

        const name = String(point.name || "").trim();
        const hash = String(point.prefix6_hex || point.token || "").trim();

        if (name !== "" && hash !== "")
        {
            return `${name}\n${hash}`;
        }

        return name || hash;
    }

    function buildPreferredPathMapPoints(preferredPath)
    {
        if (!preferredPath)
        {
            return [];
        }

        const points = [];

        if (preferredPath.source && hasValidPathNodeCoords(preferredPath.source))
        {
            points.push(
            {
                type: "source",
                name: preferredPath.source.name || "Node",
                prefix6_hex: preferredPath.source.prefix6_hex || "",
                lat: Number(preferredPath.source.adv_lat_e6) / 1000000.0,
                lon: Number(preferredPath.source.adv_lon_e6) / 1000000.0,
                distance_m: null
            });
        }

        const hops = Array.isArray(preferredPath.hops) ? preferredPath.hops : [];

        hops.forEach(function(hop)
        {
            if (hop && hop.resolved && hop.node && hasValidPathNodeCoords(hop.node))
            {
                points.push(
                {
                    type: "hop",
                    hop_index: Number(hop.hop_index),
                    name: hop.node.name || hop.node.prefix6_hex || `Hop ${hop.hop_index}`,
                    prefix6_hex: hop.node.prefix6_hex || "",
                    lat: Number(hop.node.adv_lat_e6) / 1000000.0,
                    lon: Number(hop.node.adv_lon_e6) / 1000000.0,
                    token: hop.token || "",
                    distance_m: Number.isFinite(Number(hop.distance_m)) ? Number(hop.distance_m) : null
                });
            }
        });

        if (preferredPath.endpoint && hasValidPathEndpointCoords(preferredPath.endpoint))
        {
            points.push(
            {
                type: "endpoint",
                name: preferredPath.endpoint.name || "Endpoint",
                lat: Number(preferredPath.endpoint.latitude_e6) / 1000000.0,
                lon: Number(preferredPath.endpoint.longitude_e6) / 1000000.0,
                distance_m: null
            });
        }

        return points;
    }


    function getPathSegmentDistanceMeters(fromPoint, toPoint)
    {
        const rawExplicitDistance = fromPoint ? fromPoint.distance_m : null;

        if (rawExplicitDistance !== null && rawExplicitDistance !== undefined && rawExplicitDistance !== "")
        {
            const explicitDistance = Number(rawExplicitDistance);

            if (Number.isFinite(explicitDistance))
            {
                return explicitDistance;
            }
        }

        if (!fromPoint || !toPoint || typeof Shared.distanceMeters !== "function")
        {
            return null;
        }

        const calculatedDistance = Shared.distanceMeters(
            Math.round(Number(fromPoint.lat) * 1000000.0),
            Math.round(Number(fromPoint.lon) * 1000000.0),
            Math.round(Number(toPoint.lat) * 1000000.0),
            Math.round(Number(toPoint.lon) * 1000000.0)
        );

        return Number.isFinite(calculatedDistance) ? calculatedDistance : null;
    }

    function renderPathOnCurrentMap(preferredPath)
    {
        const map = ensureMobileMap();

        if (!map || !state.leafletLayer)
        {
            return false;
        }

        const points = buildPreferredPathMapPoints(preferredPath);

        if (points.length === 0)
        {
            return false;
        }

        state.leafletLayer.clearLayers();

        const latlngs = [];

        points.forEach(function(point)
        {
            const latlng = [point.lat, point.lon];
            latlngs.push(latlng);

            const displayName = getPathPointDisplayName(point);
            let label = displayName;

            if (point.type === "source")
            {
                label = `Node: ${displayName}`;
            }
            else if (point.type === "endpoint")
            {
                label = `Endpoint: ${displayName}`;
            }
            else
            {
                label = `Hop ${point.hop_index}: ${displayName}`;
            }

            const circle = L.circleMarker(latlng,
            {
                radius: point.type === "source" ? 8 : 6,
                color: "#0a203bff",
                weight: 2,
                fillColor: "#60a5fa",
                fillOpacity: 0.8
            }).addTo(state.leafletLayer);

            circle.bindPopup(escapeHtml(label));

            const tooltipText = getPathPointTooltipText(point);

            if (state.pathLabelsVisible && tooltipText !== "")
            {
                circle.bindTooltip(escapeHtml(tooltipText),
                {
                    permanent: true,
                    direction: "center",
                    offset: [0, -1],
                    className: "path-name-label"
                });
            }
        });

        if (latlngs.length >= 2)
        {
            for (let index = 0; index < points.length - 1; index += 1)
            {
                const fromPoint = points[index];
                const toPoint = points[index + 1];

                L.polyline(
                [
                    [fromPoint.lat, fromPoint.lon],
                    [toPoint.lat, toPoint.lon]
                ],
                {
                    weight: 2
                }).addTo(state.leafletLayer);

                const segmentDistanceM = getPathSegmentDistanceMeters(fromPoint, toPoint);

                if (state.pathLabelsVisible && segmentDistanceM !== null)
                {
                    const midLat = (fromPoint.lat + toPoint.lat) / 2.0;
                    const midLon = (fromPoint.lon + toPoint.lon) / 2.0;
                    const distanceKm = segmentDistanceM / 1000.0;

                    L.marker([midLat, midLon],
                    {
                        interactive: false,
                        icon: L.divIcon(
                        {
                            className: "path-distance-label",
                            html: `<div>${distanceKm.toFixed(1)} km</div>`
                        })
                    }).addTo(state.leafletLayer);
                }
            }
        }

        state.currentMapMode = "path";
        state.currentMapPreferredPath = preferredPath;
        updateMapLabelsButton();

        setTimeout(function()
        {
            map.invalidateSize();

            if (latlngs.length === 1)
            {
                map.setView(latlngs[0], 13);
            }
            else
            {
                map.fitBounds(latlngs, { padding: [30, 30] });
            }
        }, 0);

        return true;
    }

    function updateMapLabelsButton()
    {
        const labelsButton = document.getElementById("mobileMapLabelsButton");

        if (!labelsButton)
        {
            return;
        }

        const enabled = state.currentMapMode === "path" && state.currentMapPreferredPath;
        labelsButton.disabled = !enabled;
        labelsButton.textContent = state.pathLabelsVisible ? tr("path.labels_on", "Beschriftung ein") : tr("path.labels_off", "Beschriftung aus");
        labelsButton.classList.toggle("active", Boolean(enabled && state.pathLabelsVisible));
    }

    function togglePathLabels()
    {
        if (state.currentMapMode !== "path" || !state.currentMapPreferredPath)
        {
            return;
        }

        state.pathLabelsVisible = !state.pathLabelsVisible;
        renderPathOnCurrentMap(state.currentMapPreferredPath);
        updateMapLabelsButton();
    }

    async function showNodeAdvertPath(row)
    {
        if (!row || !row.id)
        {
            return;
        }

        const pathText = String(row.last_advert_path_text || "").trim();

        if (pathText === "")
        {
            window.alert(tr("path.no_contact_path", "Für diesen Kontakt ist kein Pfad vorhanden."));
            return;
        }

        const pathButton = document.getElementById("mobileMapPathButton");

        try
        {
            if (pathButton)
            {
                pathButton.disabled = true;
                pathButton.textContent = tr("common.loading", "lade…");
            }

            const data = await Api.loadNodePath(row.id);
            const endpoint = data.endpoint || null;
            const pathEntry = data.path || null;

            if (!pathEntry)
            {
                window.alert(tr("path.no_contact_path_found", "Für diesen Kontakt wurde kein Pfad gefunden."));
                return;
            }

            if (typeof Shared.resolvePathBestRoute !== "function" || typeof Shared.buildResolvedPathListEntry !== "function")
            {
                window.alert(tr("path.logic_missing", "Die Pfadlogik wurde nicht geladen."));
                return;
            }

            const sourceNode = data.node || row || null;
            const resolvedPath = Shared.resolvePathBestRoute(pathEntry, endpoint, sourceNode);
            const preferredPath = Shared.buildResolvedPathListEntry(resolvedPath, endpoint);

            preferredPath.source =
            {
                name: data.node?.name || row.name || "",
                prefix6_hex: data.node?.prefix6_hex || row.prefix6_hex || "",
                adv_lat_e6: data.node?.adv_lat_e6 ?? row.adv_lat_e6 ?? null,
                adv_lon_e6: data.node?.adv_lon_e6 ?? row.adv_lon_e6 ?? null
            };

            const rendered = renderPathOnCurrentMap(preferredPath);

            if (!rendered)
            {
                window.alert(tr("path.no_gps_points", "Der Pfad enthält keine darstellbaren GPS-Punkte."));
                return;
            }

            const info = document.getElementById("mobileMapInfo");

            if (info)
            {
                const shownPathText = preferredPath.path_text || pathText || "-";
                info.innerHTML = `
                    <strong>${escapeHtml(row.name || "-")} · Pfad</strong>
                    <span>${escapeHtml(shownPathText)}</span>
                `;
            }
        }
        catch (error)
        {
            console.error("Advert-Pfad konnte nicht geladen werden:", error);
            window.alert(tr("path.load_failed", "Pfad konnte nicht geladen werden: {message}", { message: error.message || tr("error.unknown", "Unbekannter Fehler") }));
        }
        finally
        {
            if (pathButton)
            {
                pathButton.disabled = false;
                pathButton.textContent = tr("path.label", "Pfad");
            }
        }
    }

    function hasChatMessages(row)
    {
        return Number(row && row.msg_count || 0) > 0 || Number(row && row.newest_msg_epoch || 0) > 0;
    }

    function getContactSortTime(row)
    {
        const values = [
            normalizeGuiEpochSeconds(row && row.newest_msg_epoch || 0),
            normalizeGuiEpochSeconds(row && row.last_advert_epoch || 0),
            normalizeGuiEpochSeconds(row && row.updated_epoch || 0),
            parseGuiDateTimeEpochSeconds(row && row.last_advert_at)
        ];

        return Math.max.apply(null, values);
    }

    function sortContactNodes(nodes)
    {
        const sortMode = state.contactFilters.sort || "time_desc";

        return nodes.slice().sort(function(a, b)
        {
            if (sortMode === "name")
            {
                return String(a && a.name || "").localeCompare(String(b && b.name || ""), undefined, { sensitivity: "base" });
            }

            const timeA = getContactSortTime(a);
            const timeB = getContactSortTime(b);

            if (timeA === timeB)
            {
                return String(a && a.name || "").localeCompare(String(b && b.name || ""), undefined, { sensitivity: "base" });
            }

            if (sortMode === "time_asc")
            {
                return timeA - timeB;
            }

            return timeB - timeA;
        });
    }

    function getFilteredContactNodes()
    {
        const filteredNodes = state.nodes.filter(function(row)
        {
            const typeKey = getNodeTypeKey(row);

            if (!["chat", "repeater", "room", "sensor"].includes(typeKey))
            {
                return false;
            }

            if (state.contactFilters.type !== "all" && typeKey !== state.contactFilters.type)
            {
                return false;
            }

            if (state.contactFilters.activeOnly && !hasChatMessages(row))
            {
                return false;
            }

            if (state.contactFilters.localOnly && Number(row.is_local || 0) !== 1)
            {
                return false;
            }

            return true;
        });

        return sortContactNodes(filteredNodes);
    }

    function getNodeMeta(row)
    {
        const parts = [];

        if (row.last_advert_at)
        {
            parts.push(formatNormalizedGuiDateTime(row.last_advert_at, row.last_advert_epoch));
        }

        if (getNodePosition(row))
        {
            parts.push("GPS");
        }

        if (Number(row.msg_count || 0) > 0)
        {
            parts.push(`${Number(row.msg_count)} Msg`);
        }

        return parts;
    }

    function renderContacts()
    {
        state.view = "contacts";
        state.currentChat = null;
        state.currentMapRow = null;
        stopRefreshTimer();
        showComposer(false);
        showBack(false);
        showNav(true);
        destroyMobileMap();
        setNavActive("contacts");
        setTitle("MeshCore", tr("contacts.title", "Kontakte"));

        const visibleNodes = getFilteredContactNodes();
        const activeCount = state.nodes.filter(hasChatMessages).length;
        const typeOptions = [
            ["all", tr("filter.all", "Alle")],
            ["chat", "Chat"],
            ["repeater", tr("type.repeater", "Repeater")],
            ["room", tr("type.room", "Room")],
            ["sensor", tr("type.sensor", "Sensor")]
        ];
        const sortOptions = [
            ["time_desc", tr("sort.time_desc", "Zeit ↓")],
            ["time_asc", tr("sort.time_asc", "Zeit ↑")],
            ["name", tr("sort.name_az", "Name A-Z")]
        ];

        const filterHtml = `
            <section class="contact-filter-bar" aria-label="${escapeHtml(tr("contacts.filter_label", "Kontaktfilter"))}">
                <div class="contact-filter-row">
                    <select id="mobileContactTypeFilter" class="contact-filter-select">
                        ${typeOptions.map(function(item)
                        {
                            const selected = state.contactFilters.type === item[0] ? "selected" : "";
                            return `<option value="${escapeHtml(item[0])}" ${selected}>${escapeHtml(item[1])}</option>`;
                        }).join("")}
                    </select>

                    <select id="mobileContactSort" class="contact-filter-select contact-sort-select">
                        ${sortOptions.map(function(item)
                        {
                            const selected = state.contactFilters.sort === item[0] ? "selected" : "";
                            return `<option value="${escapeHtml(item[0])}" ${selected}>${escapeHtml(item[1])}</option>`;
                        }).join("")}
                    </select>

                    <label class="active-toggle ${state.contactFilters.activeOnly ? "active" : ""}">
                        <input type="checkbox" id="mobileActiveContactsOnly" ${state.contactFilters.activeOnly ? "checked" : ""}>
                        <span>nur aktive</span>
                        <strong>${Number(activeCount)}</strong>
                    </label>

                    <label class="active-toggle ${state.contactFilters.localOnly ? "active" : ""}">
                        <input type="checkbox" id="mobileLocalContactsOnly" ${state.contactFilters.localOnly ? "checked" : ""}>
                        <span>Local</span>
                    </label>
                </div>
            </section>
        `;

        if (visibleNodes.length === 0)
        {
            el.view.innerHTML = `${filterHtml}<div class="mobile-empty">${escapeHtml(tr("mobile.no_contacts", "Keine passenden Kontakte gefunden."))}</div>`;
            bindContactFilterEvents();
            return;
        }

        el.view.innerHTML = `${filterHtml}<div class="mobile-list">${visibleNodes.map(function(row, index)
        {
            const meta = getNodeMeta(row).map(escapeHtml).join(" · ");
            const newestEpoch = Number(row.newest_msg_epoch || 0);
            const lastRead = typeof Shared.getContactLastReadEpoch === "function"
                ? Shared.getContactLastReadEpoch(row)
                : 0;
            const unread = newestEpoch > lastRead;
            const activeContact = hasChatMessages(row);

            return `
                <article class="mobile-card contact-card ${activeContact ? "has-messages" : ""}" data-contact-index="${index}">
                    <button type="button" class="contact-open-button" data-contact-open="${index}">
                        <div class="card-head">
                            <div class="card-name">${escapeHtml(row.name || "-")}</div>
                            <div class="card-type type-${escapeHtml(getNodeTypeKey(row))}">${escapeHtml(getNodeTypeLabel(row))}</div>
                        </div>
                        <div class="card-meta">
                            ${meta || escapeHtml(tr("mobile.no_meta", "Keine Details"))}
                            ${activeContact ? `<span class="badge badge-active">aktiv</span>` : ""}
                            ${unread ? `<span class="badge">neu</span>` : ""}
                        </div>
                    </button>
                    <button type="button" class="contact-map-button" data-contact-map="${index}" aria-label="${escapeHtml(tr("map.show_position", "Position auf Karte anzeigen"))}">⌖</button>
                </article>
            `;
        }).join("")}</div>`;

        bindContactFilterEvents();

        for (const button of el.view.querySelectorAll("[data-contact-open]"))
        {
            button.addEventListener("click", function()
            {
                const row = visibleNodes[Number(button.dataset.contactOpen)];

                if (isNodeInfoOnly(row))
                {
                    renderNodeInfo(row);
                    return;
                }

                openChat(row);
            });
        }

        for (const button of el.view.querySelectorAll("[data-contact-map]"))
        {
            button.addEventListener("click", function(event)
            {
                event.stopPropagation();
                const row = visibleNodes[Number(button.dataset.contactMap)];
                showMapForContact(row);
            });
        }
    }

    function bindContactFilterEvents()
    {
        const typeSelect = document.getElementById("mobileContactTypeFilter");

        if (typeSelect)
        {
            typeSelect.addEventListener("change", function()
            {
                state.contactFilters.type = typeSelect.value || "all";
                renderContacts();
            });
        }

        const sortSelect = document.getElementById("mobileContactSort");

        if (sortSelect)
        {
            sortSelect.addEventListener("change", function()
            {
                state.contactFilters.sort = sortSelect.value || "time_desc";
                renderContacts();
            });
        }

        const activeOnly = document.getElementById("mobileActiveContactsOnly");

        if (activeOnly)
        {
            activeOnly.addEventListener("change", function()
            {
                state.contactFilters.activeOnly = activeOnly.checked;
                renderContacts();
            });
        }

        const localOnly = document.getElementById("mobileLocalContactsOnly");

        if (localOnly)
        {
            localOnly.addEventListener("change", function()
            {
                state.contactFilters.localOnly = localOnly.checked;
                renderContacts();
            });
        }
    }



    function ensureMobileMap()
    {
        if (!global.L)
        {
            return null;
        }

        if (!state.leafletMap)
        {
            state.leafletMap = L.map("mobileMap",
            {
                zoomControl: true
            });

            L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",
            {
                maxZoom: 19,
                attribution: "&copy; OpenStreetMap"
            }).addTo(state.leafletMap);

            state.leafletLayer = L.layerGroup().addTo(state.leafletMap);
        }

        return state.leafletMap;
    }

    function renderMapForContact(row)
    {
        const pos = getNodePosition(row);

        state.view = "map";
        state.currentChat = null;
        state.currentMapRow = row || null;
        state.currentMapMode = "position";
        state.currentMapPreferredPath = null;
        state.pathLabelsVisible = true;
        state.mapReturnView = "contacts";
        stopRefreshTimer();
        showComposer(false);
        showBack(true);
        showNav(false);
        setNavActive("");
        setTitle(row && row.name ? row.name : tr("map.title", "Karte"), tr("map.position", "Position"));

        if (!pos)
        {
            el.view.innerHTML = `<div class="mobile-empty">📍 <strong>${escapeHtml(row && row.name || "-")}</strong><br>${escapeHtml(tr("map.no_position_for_node", "keine Positionsdaten verfügbar."))}</div>`;
            return;
        }

        destroyMobileMap();

        const hasPath = String(row.last_advert_path_text || "").trim() !== "";

        el.view.innerHTML = `
            <section class="mobile-map-screen">
                <div id="mobileMap" class="mobile-map"></div>
                <div class="mobile-map-actions">
                    <button type="button" id="mobileMapPathButton" class="mobile-map-action" ${hasPath ? "" : "disabled"}>${escapeHtml(tr("path.label", "Pfad"))}</button>
                    <button type="button" id="mobileMapLabelsButton" class="mobile-map-action" disabled>Beschriftung ein</button>
                </div>
                <div id="mobileMapInfo" class="mobile-map-info">
                    <strong>${escapeHtml(row.name || "-")}</strong>
                    <span>${escapeHtml(pos.lat.toFixed(6))}, ${escapeHtml(pos.lon.toFixed(6))}</span>
                </div>
            </section>
        `;

        const map = ensureMobileMap();

        if (!map || !state.leafletLayer)
        {
            el.view.innerHTML = `<div class="mobile-error">${escapeHtml(tr("map.leaflet_missing", "Leaflet wurde nicht geladen."))}</div>`;
            return;
        }

        state.leafletLayer.clearLayers();

        const markerOptions = {};
        const icon = createLeafletIcon(row);

        if (icon)
        {
            markerOptions.icon = icon;
        }

        const marker = L.marker([pos.lat, pos.lon], markerOptions).addTo(state.leafletLayer);
        marker.bindPopup(escapeHtml(row.name || "-")).openPopup();
        map.setView([pos.lat, pos.lon], 13);

        const pathButton = document.getElementById("mobileMapPathButton");

        if (pathButton)
        {
            pathButton.addEventListener("click", function()
            {
                showNodeAdvertPath(row);
            });
        }

        const labelsButton = document.getElementById("mobileMapLabelsButton");

        if (labelsButton)
        {
            labelsButton.addEventListener("click", togglePathLabels);
        }

        updateMapLabelsButton();

        setTimeout(function()
        {
            map.invalidateSize();
        }, 0);
    }

    function renderMapForMessagePath(correlationKey, preferredPath)
    {
        state.view = "messagePathMap";
        state.currentMapRow = null;
        state.currentMapMode = "path";
        state.currentMapPreferredPath = preferredPath;
        state.pathLabelsVisible = true;
        state.mapReturnView = "chat";
        stopRefreshTimer();
        showComposer(false);
        showBack(true);
        showNav(false);
        setNavActive("");
        setTitle(state.currentChat && state.currentChat.name ? state.currentChat.name : tr("chat.path", "Message-Pfad"), tr("map.title", "Karte"));

        destroyMobileMap();

        el.view.innerHTML = `
            <section class="mobile-map-screen">
                <div id="mobileMap" class="mobile-map"></div>
                <div class="mobile-map-actions">
                    <button type="button" id="mobileMapLabelsButton" class="mobile-map-action" disabled>Beschriftung ein</button>
                </div>
            </section>
        `;

        const map = ensureMobileMap();

        if (!map || !state.leafletLayer)
        {
            el.view.innerHTML = `<div class="mobile-error">${escapeHtml(tr("map.leaflet_missing", "Leaflet wurde nicht geladen."))}</div>`;
            return;
        }

        const labelsButton = document.getElementById("mobileMapLabelsButton");

        if (labelsButton)
        {
            labelsButton.addEventListener("click", togglePathLabels);
        }

        const rendered = renderPathOnCurrentMap(preferredPath);

        if (!rendered)
        {
            el.view.innerHTML = `<div class="mobile-empty">${escapeHtml(tr("path.no_gps_points", "Der Pfad enthält keine darstellbaren GPS-Punkte."))}</div>`;
            return;
        }
    }

    function showMapForContact(row)
    {
        if (!row)
        {
            return;
        }

        renderMapForContact(row);
    }

    function renderNodeInfo(row)
    {
        if (!row)
        {
            return;
        }

        const pos = getNodePosition(row);
        const hasPosition = Boolean(pos);
        const lastAdvert = row.last_advert_at
            ? formatNormalizedGuiDateTime(row.last_advert_at, row.last_advert_epoch)
            : "-";
        const updatedAt = row.updated_at
            ? formatNormalizedGuiDateTime(row.updated_at, row.updated_epoch)
            : "-";
        const firstSeen = row.first_seen_at
            ? formatNormalizedGuiDateTime(row.first_seen_at, row.first_seen_epoch)
            : "-";
        const pathText = row.last_advert_path_text || "-";

        state.view = "node-info";
        state.currentChat = null;
        state.currentMapRow = null;
        stopRefreshTimer();
        showComposer(false);
        showBack(true);
        showNav(false);
        destroyMobileMap();
        setNavActive("");
        setTitle(row.name || "Node", getNodeTypeLabel(row));

        el.view.innerHTML = `
            <section class="node-info-screen">
                <div class="node-info-card">
                    <div class="node-info-head">
                        <div>
                            <div class="node-info-name">${escapeHtml(row.name || "-")}</div>
                            <div class="node-info-type type-${escapeHtml(getNodeTypeKey(row))}">${escapeHtml(getNodeTypeLabel(row))}</div>
                        </div>
                    </div>

                    <dl class="node-info-list">
                        <div>
                            <dt>Node-ID</dt>
                            <dd>${escapeHtml(formatNodeInfoValue(row.node_id))}</dd>
                        </div>
                        <div>
                            <dt>Public Key</dt>
                            <dd>${escapeHtml(formatNodeHex(row.public_key_hex))}</dd>
                        </div>
                        <div>
                            <dt>Prefix</dt>
                            <dd>${escapeHtml(formatNodeInfoValue(row.prefix6_hex))}</dd>
                        </div>
                        <div>
                            <dt>Last Advert</dt>
                            <dd>${escapeHtml(lastAdvert)}</dd>
                        </div>
                        <div>
                            <dt>First Seen</dt>
                            <dd>${escapeHtml(firstSeen)}</dd>
                        </div>
                        <div>
                            <dt>Updated</dt>
                            <dd>${escapeHtml(updatedAt)}</dd>
                        </div>
                        <div>
                            <dt>Position</dt>
                            <dd>${hasPosition ? `${escapeHtml(pos.lat.toFixed(6))}, ${escapeHtml(pos.lon.toFixed(6))}` : "-"}</dd>
                        </div>
                        <div>
                            <dt>Path</dt>
                            <dd>${escapeHtml(pathText)}</dd>
                        </div>
                        <div>
                            <dt>${escapeHtml(tr("node.messages", "Messages"))}</dt>
                            <dd>${escapeHtml(formatNodeInfoValue(row.msg_count, "0"))}</dd>
                        </div>
                    </dl>

                    <div class="node-info-actions">
                        <button type="button" id="nodeInfoMapButton" ${hasPosition ? "" : "disabled"}>${escapeHtml(tr("map.show_position", "Position auf Karte anzeigen"))}</button>
                    </div>
                </div>
            </section>
        `;

        const mapButton = document.getElementById("nodeInfoMapButton");

        if (mapButton)
        {
            mapButton.addEventListener("click", function()
            {
                showMapForContact(row);
            });
        }
    }

    function renderChannels()
    {
        state.view = "channels";
        state.currentChat = null;
        state.currentMapRow = null;
        stopRefreshTimer();
        showComposer(false);
        showBack(false);
        showNav(true);
        destroyMobileMap();
        setNavActive("channels");
        setTitle("MeshCore", tr("tabs.channels", "Channels"));

        const actionSelect = `
            <div class="mobile-channel-toolbar">
                <select id="mobileChannelActionSelect" class="mobile-channel-action-select" aria-label="${escapeHtml(tr("channel.action.placeholder", "Action..."))}">
                    <option value="">${escapeHtml(tr("channel.action.placeholder", "Action..."))}</option>
                    <option value="create_private">${escapeHtml(tr("channel.button.create_private", "Create Private"))}</option>
                    <option value="join_private">${escapeHtml(tr("channel.button.join_private", "Join Private"))}</option>
                    <option value="join_public">${escapeHtml(tr("channel.button.join_public", "Join Public"))}</option>
                    <option value="join_hashtag">${escapeHtml(tr("channel.button.join_hashtag", "Join Hashtag"))}</option>
                    <option value="remove">${escapeHtml(tr("channel.button.remove", "Remove"))}</option>
                </select>
            </div>
        `;

        if (state.channels.length === 0)
        {
            el.view.innerHTML = actionSelect + `<div class="mobile-empty">${escapeHtml(tr("mobile.no_channels", "Keine Channels gefunden."))}</div>`;
            bindMobileChannelActionSelect();
            return;
        }

        el.view.innerHTML = actionSelect + `<div class="mobile-list">${state.channels.map(function(row, index)
        {
            const enabled = row.enabled ? tr("channel.enabled", "aktiv") : tr("channel.disabled", "inaktiv");
            const context = row.has_local_context ? tr("channel.send_context", "Sendekontext") : tr("channel.display_only", "nur Anzeige");

            return `
                <button type="button" class="mobile-card" data-channel-index="${index}">
                    <div class="card-head">
                        <div class="card-name">${escapeHtml(row.name || "Channel")}</div>
                        <div class="card-type">Channel</div>
                    </div>
                    <div class="card-meta">${escapeHtml(enabled)} · ${escapeHtml(context)}</div>
                </button>
            `;
        }).join("")}</div>`;

        bindMobileChannelActionSelect();

        for (const button of el.view.querySelectorAll("[data-channel-index]"))
        {
            button.addEventListener("click", function()
            {
                const row = Object.assign({ type: "channel" }, state.channels[Number(button.dataset.channelIndex)]);
                openChat(row);
            });
        }
    }


    function bindMobileChannelActionSelect()
    {
        const select = document.getElementById("mobileChannelActionSelect");

        if (!select)
        {
            return;
        }

        select.addEventListener("change", function()
        {
            const action = select.value;
            select.value = "";

            if (!action)
            {
                return;
            }

            openMobileChannelDialog(action);
        });
    }

    function setChannelModalError(message)
    {
        if (!el.channelModalError)
        {
            return;
        }

        el.channelModalError.textContent = message || "";
        el.channelModalError.style.display = message ? "" : "none";
    }

    function fillChannelRemoveSelect()
    {
        if (!el.channelRemoveSelect)
        {
            return;
        }

        el.channelRemoveSelect.innerHTML = "";

        for (const channel of state.channels)
        {
            const option = document.createElement("option");
            const idx = Number(channel.channel_idx || 0);
            const name = channel.name || tr("channel.fallback_name", "Channel {idx}", { idx: idx });
            option.value = String(channel.key_hex || "");
            option.textContent = `${name} (IDX ${idx})`;
            el.channelRemoveSelect.appendChild(option);
        }
    }

    function renderMobileChannelQrCode(payload)
    {
        if (!el.channelQrCode)
        {
            return;
        }

        el.channelQrCode.innerHTML = "";

        if (typeof global.QRCode === "undefined")
        {
            el.channelQrCode.textContent = tr("error.qr_library_missing", "QR-Code Bibliothek nicht geladen.");
            return;
        }

        new global.QRCode(el.channelQrCode,
        {
            text: payload,
            width: 200,
            height: 200,
            correctLevel: global.QRCode.CorrectLevel.M
        });
    }

    function openMobileChannelDialog(action)
    {
        state.channelDialog = { action: action };

        setChannelModalError("");

        if (el.channelNameInput)
        {
            el.channelNameInput.value = "";
        }

        if (el.channelSecretInput)
        {
            el.channelSecretInput.value = "";
        }

        if (el.channelResultSecret)
        {
            el.channelResultSecret.value = "";
        }

        if (el.channelQrCode)
        {
            el.channelQrCode.innerHTML = "";
        }

        if (el.channelNameGroup)
        {
            el.channelNameGroup.style.display = "";
        }

        if (el.channelSecretGroup)
        {
            el.channelSecretGroup.style.display = "none";
        }

        if (el.channelRemoveGroup)
        {
            el.channelRemoveGroup.style.display = "none";
        }

        if (el.channelResultGroup)
        {
            el.channelResultGroup.style.display = "none";
        }

        if (el.channelCancelButton)
        {
            el.channelCancelButton.style.display = "";
            el.channelCancelButton.disabled = false;
        }

        if (el.channelConfirmButton)
        {
            el.channelConfirmButton.disabled = false;
        }

        switch (action)
        {
            case "create_private":
                el.channelModalTitle.textContent = tr("channel.action.create_private.title", "Create private channel");
                el.channelModalSubtitle.textContent = tr("channel.action.create_private.subtitle", "Neuen privaten Channel anlegen. Der Secret Key wird anschließend angezeigt.");
                el.channelConfirmButton.textContent = tr("channel.action.create_private.confirm", "Create");
                break;

            case "join_private":
                el.channelModalTitle.textContent = tr("channel.action.join_private.title", "Join private channel");
                el.channelModalSubtitle.textContent = tr("channel.action.join_private.subtitle", "Channelname und Secret Key eingeben.");
                el.channelConfirmButton.textContent = tr("channel.action.join_private.confirm", "Join");
                el.channelSecretGroup.style.display = "";
                break;

            case "join_public":
                el.channelModalTitle.textContent = tr("channel.action.join_public.title", "Join public channel");
                el.channelModalSubtitle.textContent = tr("channel.action.join_public.subtitle", "Öffentlichen Channel über den Namen beitreten.");
                el.channelConfirmButton.textContent = tr("channel.action.join_public.confirm", "Join");
                break;

            case "join_hashtag":
                el.channelModalTitle.textContent = tr("channel.action.join_hashtag.title", "Join hashtag channel");
                el.channelModalSubtitle.textContent = tr("channel.action.join_hashtag.subtitle", "Hashtag-Channel eingeben, z. B. #drones.");
                el.channelConfirmButton.textContent = tr("channel.action.join_hashtag.confirm", "Join");
                break;

            case "remove":
                el.channelModalTitle.textContent = tr("channel.action.remove.title", "Remove channel");
                el.channelModalSubtitle.textContent = tr("channel.action.remove.none_selected", "Kanal zum Entfernen auswählen.");
                el.channelConfirmButton.textContent = tr("channel.action.remove.confirm", "Remove");
                el.channelNameGroup.style.display = "none";
                el.channelSecretGroup.style.display = "none";
                el.channelRemoveGroup.style.display = "";
                fillChannelRemoveSelect();
                break;

            default:
                return;
        }

        setModalVisible(el.channelModal, true);

        setTimeout(function()
        {
            if (action === "remove" && el.channelRemoveSelect)
            {
                el.channelRemoveSelect.focus();
            }
            else if (el.channelNameInput)
            {
                el.channelNameInput.focus();
            }
        }, 0);
    }

    function closeMobileChannelDialog()
    {
        state.channelDialog = null;
        setModalVisible(el.channelModal, false);
        setChannelModalError("");
    }

    function buildMobileChannelPayload()
    {
        if (!state.channelDialog)
        {
            return null;
        }

        const action = state.channelDialog.action;
        const name = el.channelNameInput ? el.channelNameInput.value.trim() : "";
        const secret = el.channelSecretInput ? el.channelSecretInput.value.trim() : "";

        if (action === "create_private")
        {
            return { action: "create_private", name: name };
        }

        if (action === "join_private")
        {
            return { action: "join_private", name: name, secret_key: secret };
        }

        if (action === "join_public")
        {
            return { action: "join_public", name: name };
        }

        if (action === "join_hashtag")
        {
            return { action: "join_hashtag", name: name };
        }

        return null;
    }

    async function handleMobileChannelConfirm()
    {
        if (!state.channelDialog)
        {
            return;
        }

        if (state.channelDialog.action === "done_after_create")
        {
            closeMobileChannelDialog();
            return;
        }

        const action = state.channelDialog.action;
        setChannelModalError("");

        if (el.channelConfirmButton)
        {
            el.channelConfirmButton.disabled = true;
        }

        if (el.channelCancelButton)
        {
            el.channelCancelButton.disabled = true;
        }

        try
        {
            if (action === "remove")
            {
                const keyHex = el.channelRemoveSelect ? el.channelRemoveSelect.value : "";

                if (!keyHex)
                {
                    throw new Error(tr("channel.none_selected", "Kein Channel ausgewählt."));
                }

                await Api.deleteChannel(keyHex);
                await loadChannels();
                renderChannels();
                closeMobileChannelDialog();
                return;
            }

            const payload = buildMobileChannelPayload();

            if (!payload)
            {
                throw new Error(tr("channel.invalid_action", "Ungültige Aktion."));
            }

            const result = await Api.saveChannel(payload);
            await loadChannels();
            renderChannels();

            if (action === "create_private" && result && result.secret_key)
            {
                if (el.channelResultSecret)
                {
                    el.channelResultSecret.value = result.secret_key;
                }

                if (el.channelResultGroup)
                {
                    el.channelResultGroup.style.display = "";
                }

                if (el.channelModalSubtitle)
                {
                    el.channelModalSubtitle.textContent = tr("channel.action.create_private.done_subtitle", "Channel wurde erstellt. Diesen Secret Key jetzt mit den anderen Teilnehmern teilen.");
                }

                const qrBuilder = Shared.buildMeshCoreChannelQrPayload;
                if (typeof qrBuilder === "function")
                {
                    renderMobileChannelQrCode(qrBuilder(result.channel?.name || payload.name || "", result.secret_key));
                }

                if (el.channelConfirmButton)
                {
                    el.channelConfirmButton.textContent = tr("common.done", "Done");
                    el.channelConfirmButton.disabled = false;
                }

                if (el.channelCancelButton)
                {
                    el.channelCancelButton.style.display = "none";
                    el.channelCancelButton.disabled = true;
                }

                state.channelDialog.action = "done_after_create";
                return;
            }

            closeMobileChannelDialog();
        }
        catch (error)
        {
            setChannelModalError(error.message || tr("error.generic", "Fehler"));
        }
        finally
        {
            if (state.channelDialog && state.channelDialog.action !== "done_after_create")
            {
                if (el.channelConfirmButton)
                {
                    el.channelConfirmButton.disabled = false;
                }

                if (el.channelCancelButton)
                {
                    el.channelCancelButton.disabled = false;
                }
            }
        }
    }

    function setModalVisible(modal, visible)
    {
        if (!modal)
        {
            return;
        }

        modal.classList.toggle("visible", !!visible);
        modal.setAttribute("aria-hidden", visible ? "false" : "true");
    }

    function showSetupError(message)
    {
        if (!el.setupModalError)
        {
            return;
        }

        el.setupModalError.textContent = message || "";
        el.setupModalError.style.display = message ? "block" : "none";
    }

    async function openSetupDialog()
    {
        if (!el.setupModal)
        {
            return;
        }

        showSetupError("");

        if (el.setupNameInput)
        {
            el.setupNameInput.value = "";
        }

        if (el.setupCityInput)
        {
            el.setupCityInput.value = "";
        }

        if (el.setupLatInput)
        {
            el.setupLatInput.value = "";
        }

        if (el.setupLonInput)
        {
            el.setupLonInput.value = "";
        }

        setModalVisible(el.setupModal, true);

        try
        {
            const data = await Api.loadCompanionSetup();
            const cfg = data && data.config ? data.config : null;

            if (cfg)
            {
                if (el.setupNameInput)
                {
                    el.setupNameInput.value = cfg.name || "";
                }

                if (el.setupCityInput)
                {
                    el.setupCityInput.value = cfg.location_name || "";
                }

                if (el.setupLatInput)
                {
                    el.setupLatInput.value = Number.isFinite(Number(cfg.latitude)) ? String(cfg.latitude) : "";
                }

                if (el.setupLonInput)
                {
                    el.setupLonInput.value = Number.isFinite(Number(cfg.longitude)) ? String(cfg.longitude) : "";
                }
            }
        }
        catch (error)
        {
            showSetupError(error.message || tr("setup.error.load_failed", "Setup-Werte konnten nicht geladen werden."));
        }

        if (el.setupNameInput)
        {
            el.setupNameInput.focus();
        }
    }

    function closeSetupDialog()
    {
        setModalVisible(el.setupModal, false);
        showSetupError("");
    }

    async function applyCompanionSetup()
    {
        const name = el.setupNameInput ? el.setupNameInput.value.trim() : "";
        const locationName = el.setupCityInput ? el.setupCityInput.value.trim() : "";
        const latText = el.setupLatInput ? el.setupLatInput.value.trim() : "";
        const lonText = el.setupLonInput ? el.setupLonInput.value.trim() : "";
        const latitude = Number(latText);
        const longitude = Number(lonText);

        if (name === "")
        {
            throw new Error(tr("setup.error.name_missing", "Name fehlt."));
        }

        if (locationName.length > 128)
        {
            throw new Error(tr("setup.error.city_too_long", "City ist zu lang."));
        }

        if (!Number.isFinite(latitude) || latitude < -90 || latitude > 90)
        {
            throw new Error(tr("setup.error.latitude_invalid", "Latitude ist ungültig."));
        }

        if (!Number.isFinite(longitude) || longitude < -180 || longitude > 180)
        {
            throw new Error(tr("setup.error.longitude_invalid", "Longitude ist ungültig."));
        }

        return await Api.applyCompanionSetup(
        {
            name: name,
            location_name: locationName,
            latitude: latitude,
            longitude: longitude
        });
    }

    function showDiscoverError(message)
    {
        if (!el.discoverModalError)
        {
            return;
        }

        el.discoverModalError.textContent = message || "";
        el.discoverModalError.style.display = message ? "block" : "none";
    }

    function renderDiscoverResults(results)
    {
        state.discoverResults = Array.isArray(results) ? results : [];

        if (!el.discoverResults)
        {
            return;
        }

        if (state.discoverResults.length === 0)
        {
            el.discoverResults.innerHTML = `<div class="mobile-discover-empty">${escapeHtml(tr("discover.results.none", "Noch keine Ergebnisse."))}</div>`;
            return;
        }

        el.discoverResults.innerHTML = state.discoverResults.map(function(row)
        {
            const name = row.node_name || row.node_id_hex || row.name || row.short_name || row.public_key_hex || row.node_id || "Repeater";
            const meta = [
                row.node_id_hex ? `ID: ${row.node_id_hex}` : null,
                row.snr_rx_db !== undefined && row.snr_rx_db !== null ? `SNR RX: ${row.snr_rx_db}` : null,
                row.snr_tx_db !== undefined && row.snr_tx_db !== null ? `SNR TX: ${row.snr_tx_db}` : null,
                row.rssi_dbm !== undefined && row.rssi_dbm !== null ? `RSSI: ${row.rssi_dbm}` : null,
                row.updated_at ? `${tr("discover.updated", "Update")}: ${row.updated_at}` : null
            ].filter(Boolean).join(" · ");

            return `
                <div class="mobile-discover-card">
                    <strong>${escapeHtml(name)}</strong>
                    <div>${escapeHtml(meta || "-")}</div>
                </div>
            `;
        }).join("");
    }

    function formatDiscoverJobInfo(job)
    {
        if (!job)
        {
            return tr("discover.job.none", "Noch kein Discover gestartet.");
        }

        const parts = [];
        parts.push(`${tr("discover.results_count", "Treffer")}: ${job.result_count || 0}`);

        if (job.finished_at)
        {
            parts.push(`${tr("discover.finished", "beendet")}: ${job.finished_at}`);
        }
        else if (job.started_at)
        {
            parts.push(`${tr("discover.started", "gestartet")}: ${job.started_at}`);
        }
        else if (job.created_at)
        {
            parts.push(`${tr("discover.created", "angelegt")}: ${job.created_at}`);
        }

        if (job.error_text)
        {
            parts.push(`${tr("discover.error_label", "Fehler")}: ${job.error_text}`);
        }

        return parts.join(" | ");
    }

    function setDiscoverStartButtonEnabled(enabled)
    {
        if (el.discoverRepeatButton)
        {
            el.discoverRepeatButton.disabled = !enabled;
            el.discoverRepeatButton.setAttribute("aria-disabled", enabled ? "false" : "true");
        }
    }

    function isDiscoverJobDone(job)
    {
        if (!job)
        {
            return false;
        }

        const statusKey = String(job.status_text || job.status || "").toLowerCase();
        return Number(job.status) === 3 || statusKey === "done";
    }

    function resetDiscoverPendingState()
    {
        state.discoverPending = false;
        state.discoverPendingJobId = null;
    }

    function renderDiscoverStatus(data)
    {
        const job = data && data.job ? data.job : null;
        const results = data && Array.isArray(data.results) ? data.results : [];
        const status = job && job.status_text ? job.status_text : tr("discover.status.unknown", "Unbekannt");

        if (el.discoverStatusText)
        {
            el.discoverStatusText.textContent = status;
        }

        if (el.discoverJobInfo)
        {
            el.discoverJobInfo.textContent = formatDiscoverJobInfo(job);
        }

        renderDiscoverResults(results);

        if (state.discoverPending)
        {
            if (!job)
            {
                setDiscoverStartButtonEnabled(false);
                return;
            }

            if (
                state.discoverPendingJobId === null ||
                Number(job.id) < Number(state.discoverPendingJobId)
            )
            {
                setDiscoverStartButtonEnabled(false);
                return;
            }

            resetDiscoverPendingState();
        }

        setDiscoverStartButtonEnabled(isDiscoverJobDone(job));
    }

    async function refreshDiscoverModal()
    {
        try
        {
            const data = await Api.loadDiscoverStatus();
            showDiscoverError("");
            renderDiscoverStatus(data);
        }
        catch (error)
        {
            showDiscoverError(error.message || tr("discover.error.status_failed", "Discover-Status konnte nicht geladen werden."));
        }
    }

    function stopDiscoverPolling()
    {
        if (state.discoverPollTimer)
        {
            clearInterval(state.discoverPollTimer);
            state.discoverPollTimer = null;
        }
    }

    function startDiscoverPolling()
    {
        stopDiscoverPolling();
        state.discoverPollTimer = setInterval(function()
        {
            refreshDiscoverModal();
        }, 2500);
    }

    async function handleDiscoverStartClick()
    {
        if (el.discoverRepeatButton && el.discoverRepeatButton.disabled)
        {
            return;
        }

        if (state.discoverPending)
        {
            return;
        }

        state.discoverPending = true;
        state.discoverPendingJobId = null;
        setDiscoverStartButtonEnabled(false);
        showDiscoverError("");

        try
        {
            const response = await Api.startDiscover();
            state.discoverPendingJobId = response && response.job_id ? Number(response.job_id) : null;
            await refreshDiscoverModal();
            startDiscoverPolling();
        }
        catch (error)
        {
            resetDiscoverPendingState();
            showDiscoverError(error.message || tr("discover.error.start_failed", "Discover konnte nicht gestartet werden."));
            await refreshDiscoverModal();
        }
    }

    async function openDiscoverDialog()
    {
        setDiscoverStartButtonEnabled(false);
        setModalVisible(el.discoverModal, true);
        showDiscoverError("");
        await refreshDiscoverModal();
        startDiscoverPolling();
    }

    async function closeDiscoverDialog()
    {
        stopDiscoverPolling();
        setModalVisible(el.discoverModal, false);

        try
        {
            if (typeof Api.clearDiscoverRequest === "function")
            {
                await Api.clearDiscoverRequest();
            }
        }
        catch (error)
        {
            console.error("Discover clear failed:", error);
        }
    }

    function refreshCurrentViewAfterLanguageChange()
    {
        const view = state.view;

        if (view === "contacts")
        {
            renderContacts();
        }
        else if (view === "channels")
        {
            renderChannels();
        }
        else if (view === "info")
        {
            renderInfo();
        }
        else if (view === "chat")
        {
            renderChatMessages(state.currentMessages || []);
        }
        else if (view === "node" && state.currentMapRow)
        {
            renderNodeInfo(state.currentMapRow);
        }
        else if (view === "map" && state.currentMapRow)
        {
            showMapForContact(state.currentMapRow);
        }
    }

    function openLanguageDialog()
    {
        if (el.languageSelect && typeof global.getLanguage === "function")
        {
            el.languageSelect.value = global.getLanguage();
        }

        setModalVisible(el.languageModal, true);
    }

    function closeLanguageDialog()
    {
        setModalVisible(el.languageModal, false);
    }

    const MOBILE_NOISE_FLOOR_CONFIG =
    {
        minDbm: -120,
        maxDbm: -70,
        greenMaxDbm: -110,
        yellowMaxDbm: -96
    };

    function mobileNoiseFloorPercent(noiseFloor)
    {
        const minDbm = MOBILE_NOISE_FLOOR_CONFIG.minDbm;
        const maxDbm = MOBILE_NOISE_FLOOR_CONFIG.maxDbm;
        const clamped = Math.max(minDbm, Math.min(maxDbm, noiseFloor));

        return ((clamped - minDbm) / (maxDbm - minDbm)) * 100.0;
    }

    function mobileNoiseFloorState(noiseFloor)
    {
        if (noiseFloor <= MOBILE_NOISE_FLOOR_CONFIG.greenMaxDbm)
        {
            return "green";
        }

        if (noiseFloor <= MOBILE_NOISE_FLOOR_CONFIG.yellowMaxDbm)
        {
            return "yellow";
        }

        return "red";
    }

    function renderMobileRadioStatus(status)
    {
        const meter = document.getElementById("mobileNoiseFloorMeter");
        const fill = document.getElementById("mobileNoiseFloorFill");
        const noiseText = document.getElementById("mobileNoiseFloorText");
        const batteryText = document.getElementById("mobileBatteryVoltageText");
        const led = document.getElementById("mobileCompanionLinkLed");

        if (!meter || !fill || !noiseText || !batteryText || !led)
        {
            return;
        }

        const updatedAt = status ? status.updated_at : null;
        const noiseFloor = status && status.noise_floor !== null ? Number(status.noise_floor) : NaN;
        const batteryMv = status && status.battery_mv !== null ? Number(status.battery_mv) : NaN;
        const connected = status ? !!status.connected : false;

        if (Number.isFinite(noiseFloor))
        {
            const stateLabel = mobileNoiseFloorState(noiseFloor);
            const percent = mobileNoiseFloorPercent(noiseFloor);

            meter.classList.add("has-value");
            meter.dataset.state = stateLabel;
            fill.className = `mobile-noise-floor-fill ${stateLabel}`;
            fill.style.width = `${percent}%`;
            noiseText.textContent = `noise: ${Math.round(noiseFloor)}dBm`;
            meter.title =
                `Noise floor: ${noiseFloor.toFixed(1)} dBm` +
                (updatedAt ? `\nUpdate: ${updatedAt}` : "");
        }
        else
        {
            meter.classList.remove("has-value");
            fill.className = "mobile-noise-floor-fill";
            fill.style.width = "0%";
            noiseText.textContent = "noise: --";
            meter.title = "Noise floor: kein Wert";
        }

        if (Number.isFinite(batteryMv))
        {
            const voltage = batteryMv / 1000.0;
            const voltageText = voltage.toFixed(3).replace(".", ",");
            batteryText.textContent = `Batt: ${voltageText} V`;
            batteryText.title =
                `Battery: ${voltage.toFixed(3)} V` +
                (updatedAt ? `\nUpdate: ${updatedAt}` : "");
        }
        else
        {
            batteryText.textContent = "Batt: --";
            batteryText.title = "Battery: kein Wert";
        }

        if (connected)
        {
            led.classList.remove("offline");
            led.classList.add("online");
            led.title =
                "Companion: verbunden" +
                (updatedAt ? `\nUpdate: ${updatedAt}` : "");
        }
        else
        {
            led.classList.remove("online");
            led.classList.add("offline");
            led.title =
                "Companion: nicht verbunden" +
                (updatedAt ? `\nLetztes Update: ${updatedAt}` : "");
        }
    }

    async function refreshMobileRadioStatus()
    {
        if (typeof Api.loadCompanionRadioStatus !== "function")
        {
            renderMobileRadioStatus(null);
            return;
        }

        try
        {
            const data = await Api.loadCompanionRadioStatus();
            renderMobileRadioStatus(data && data.status ? data.status : null);
        }
        catch (error)
        {
            console.error("Mobile radio status refresh failed:", error);
            renderMobileRadioStatus(null);
        }
    }

    function renderInfo()
    {
        state.view = "info";
        state.currentChat = null;
        state.currentMapRow = null;
        stopRefreshTimer();
        showComposer(false);
        showBack(false);
        showNav(true);
        destroyMobileMap();
        setNavActive("info");
        setTitle("MeshCore", "Mobile");

        el.view.innerHTML = `
            <div class="mobile-radio-status-card">
                <div id="mobileNoiseFloorMeter" class="mobile-noise-floor-meter" title="Noise floor">
                    <div class="mobile-noise-floor-track" aria-hidden="true">
                        <div id="mobileNoiseFloorFill" class="mobile-noise-floor-fill"></div>
                    </div>
                    <div id="mobileNoiseFloorText" class="mobile-noise-floor-text">noise: --</div>
                    <div id="mobileBatteryVoltageText" class="mobile-battery-voltage-text">Batt: --</div>
                    <div class="mobile-companion-link-status" title="Companion connection">
                        <span id="mobileCompanionLinkLed" class="mobile-companion-link-led offline"></span>
                    </div>
                </div>
            </div>
            <div class="info-action-list">
                <button type="button" id="mobileSetupButton" class="info-action-button">⚙ ${escapeHtml(tr("setup.title", "Companion Setup"))}</button>
                <button type="button" id="mobileDiscoverButton" class="info-action-button">🔎 ${escapeHtml(tr("discover.title", "Repeater Discovery"))}</button>
                <button type="button" id="mobileLanguageButton" class="info-action-button">🌐 ${escapeHtml(tr("language.button", "Sprache"))}</button>
                <button type="button" id="mobileAdvertButton" class="info-action-button">📡 ${escapeHtml(tr("toolbar.advert", "Advert"))}</button>
            </div>
        `;

        refreshMobileRadioStatus();

        document.getElementById("mobileSetupButton")?.addEventListener("click", function()
        {
            openSetupDialog();
        });

        document.getElementById("mobileDiscoverButton")?.addEventListener("click", function()
        {
            openDiscoverDialog();
        });

        document.getElementById("mobileLanguageButton")?.addEventListener("click", function()
        {
            openLanguageDialog();
        });

        document.getElementById("mobileAdvertButton")?.addEventListener("click", function(event)
        {
            sendMobileFloodAdvert(event.currentTarget);
        });
    }

    async function showMessagePath(correlationKey, triggerButton)
    {
        const key = String(correlationKey || "").trim();

        if (key === "")
        {
            return;
        }

        const originalText = triggerButton ? triggerButton.textContent : "";

        try
        {
            if (triggerButton)
            {
                triggerButton.disabled = true;
                triggerButton.textContent = "lade…";
            }

            const data = await Api.loadMessagePath(key);
            const paths = Array.isArray(data.paths) ? data.paths : [];
            const endpoint = data.endpoint || null;

            if (paths.length === 0)
            {
                window.alert(tr("chat.no_path_for_message", "Für diese Message wurde kein Pfad gefunden."));
                return;
            }

            if (typeof Shared.buildResolvedPathList !== "function" || typeof Shared.selectPreferredResolvedPath !== "function")
            {
                window.alert(tr("path.logic_missing", "Die Pfadlogik wurde nicht geladen."));
                return;
            }

            const resolvedPaths = Shared.buildResolvedPathList(paths, endpoint);
            const preferredPath = Shared.selectPreferredResolvedPath(resolvedPaths);

            if (!preferredPath)
            {
                window.alert(tr("chat.path_unresolved", "Der Message-Pfad konnte nicht aufgelöst werden."));
                return;
            }

            renderMapForMessagePath(key, preferredPath);
        }
        catch (error)
        {
            console.error("Message-Pfad konnte nicht geladen werden:", error);
            window.alert(tr("chat.path_load_failed", "Message-Pfad konnte nicht geladen werden: {message}", { message: error.message || tr("error.unknown", "Unbekannter Fehler") }));
        }
        finally
        {
            if (triggerButton)
            {
                triggerButton.disabled = false;
                triggerButton.textContent = originalText || "⤳";
            }
        }
    }

    function bindChatPathButtons()
    {
        for (const button of el.view.querySelectorAll("[data-message-path]"))
        {
            button.addEventListener("click", function()
            {
                showMessagePath(button.getAttribute("data-message-path") || "", button);
            });
        }
    }

    function renderChatMessages(messages)
    {
        if (!Array.isArray(messages) || messages.length === 0)
        {
            el.view.innerHTML = `<div class="mobile-empty">${escapeHtml(tr("chat.empty", "Keine Messages für diesen Eintrag gefunden."))}</div>`;
            return;
        }

        el.view.innerHTML = `<div class="chat-list">${messages.map(function(msg)
        {
            const outgoing = Number(msg.direction || 0) === 1;
            const dateText = escapeHtml(formatDate(msg.received_at, msg.timestamp_epoch));
            const textValue = formatMessageText(msg.text || msg.message_text || "");
            const statusClass = outgoing && typeof Shared.getOutgoingStatusClass === "function"
                ? Shared.getOutgoingStatusClass(msg)
                : "";
            const statusText = outgoing
                ? escapeHtml(String(msg.ui_status_text || tr("chat.status.pending", "Pending")))
                : "";
            const roomSender = String(msg.room_sender_name || "").trim();
            const senderText = !outgoing && roomSender !== "" ? `${escapeHtml(roomSender)} · ` : "";
            const correlationKey = String(msg.correlation_key || "").trim();
            const pathButton = correlationKey !== ""
                ? `<button type="button" class="chat-path-button" data-message-path="${escapeHtml(correlationKey)}" title="${escapeHtml(tr("chat.show_path", "Pfad anzeigen"))}">⤳</button>`
                : "";

            return `
                <div class="chat-message ${outgoing ? "outgoing" : "incoming"}">
                    <div class="chat-meta">
                        <span>${senderText}${dateText}</span>
                        ${pathButton}
                    </div>
                    <div class="chat-text">${textValue}</div>
                    ${outgoing ? `<div class="chat-status ${escapeHtml(statusClass)}">${statusText}</div>` : ""}
                </div>
            `;
        }).join("")}</div>`;

        bindChatPathButtons();
    }

    async function loadNodes()
    {
        const data = await Api.loadNodes("all");
        state.nodes = Array.isArray(data.nodes) ? data.nodes : [];
    }

    async function loadChannels()
    {
        const data = await Api.loadChannels();
        state.channels = Array.isArray(data.channels) ? data.channels : [];
    }

    async function loadChatMessages(row, keepScroll)
    {
        const data = await Api.loadMessages(
        {
            kind: getChatKindValue(row),
            name: row.name || getChatKindLabel(row),
            channel_key_hex: row.key_hex || ""
        });

        const messages = Array.isArray(data.messages) ? data.messages : [];
        const oldSerialized = JSON.stringify(state.currentMessages);
        const newSerialized = JSON.stringify(messages);

        if (oldSerialized === newSerialized)
        {
            return;
        }

        const wasNearBottom = el.view.scrollHeight - el.view.scrollTop - el.view.clientHeight < 80;
        state.currentMessages = messages;
        renderChatMessages(messages);

        if (getChatKindValue(row) === "channel")
        {
            const newest = messages.length > 0 ? Number(messages[messages.length - 1].timestamp_epoch || 0) : 0;

            if (newest > 0 && typeof Shared.markChannelAsRead === "function")
            {
                Shared.markChannelAsRead(
                {
                    key_hex: row.key_hex,
                    newest_message_epoch: newest
                });
            }
        }
        else
        {
            const newest = messages.length > 0 ? Number(messages[messages.length - 1].timestamp_epoch || 0) : 0;

            if (newest > 0 && typeof Shared.markContactAsRead === "function")
            {
                Shared.markContactAsRead(Object.assign({}, row,
                {
                    newest_msg_epoch: newest
                }));
            }
        }

        if (!keepScroll || wasNearBottom)
        {
            requestAnimationFrame(function()
            {
                el.view.scrollTop = el.view.scrollHeight;
            });
        }
    }

    async function openChat(row)
    {
        if (!row)
        {
            return;
        }

        state.currentChat = row;
        state.currentMapRow = null;
        state.currentMessages = [];
        state.view = "chat";
        stopRefreshTimer();
        showComposer(true);
        showBack(true);
        setNavActive("");
        setTitle(row.name || getChatKindLabel(row), getChatKindLabel(row));
        el.input.value = "";
        el.view.innerHTML = `<div class="mobile-empty">${escapeHtml(tr("chat.loading", "Lade Messages ..."))}</div>`;

        try
        {
            await loadChatMessages(row, false);
            startRefreshTimer();
        }
        catch (error)
        {
            showError(tr("chat.error_loading", "Fehler beim Laden der Messages: {message}",
            {
                message: error.message || tr("error.unknown", "Unbekannter Fehler")
            }));
        }
    }

    async function sendMessage()
    {
        if (!state.currentChat || state.busy)
        {
            return;
        }

        const text = String(el.input.value || "").trim();

        if (text === "")
        {
            return;
        }

        if (isChannelRow(state.currentChat))
        {
            if (!state.currentChat.enabled)
            {
                window.alert(tr("channel.send.disabled", "Dieser Channel ist deaktiviert."));
                return;
            }

            if (!state.currentChat.has_local_context)
            {
                window.alert(tr("channel.send.no_local_context", "Für diesen Channel ist kein lokaler Sendekontext konfiguriert."));
                return;
            }
        }

        setBusy(true);

        try
        {
            await Api.sendMessage(buildOutgoingPayload(state.currentChat, text));
            el.input.value = "";
            await loadChatMessages(state.currentChat, false);
        }
        catch (error)
        {
            window.alert(tr("chat.send_error", "Fehler beim Senden: {message}",
            {
                message: error.message || tr("error.unknown", "Unbekannter Fehler")
            }));
        }
        finally
        {
            setBusy(false);
            el.input.focus();
        }
    }

    async function refreshCurrentView()
    {
        if (state.busy)
        {
            return;
        }

        setBusy(true);

        try
        {
            if (state.view === "chat" && state.currentChat)
            {
                await loadChatMessages(state.currentChat, false);
                return;
            }

            if (state.view === "channels")
            {
                await loadChannels();
                renderChannels();
                return;
            }

            if (state.view === "map" && state.currentMapRow)
            {
                await loadNodes();
                const currentId = String(state.currentMapRow.id || state.currentMapRow.node_id || "");
                const freshRow = state.nodes.find(function(row)
                {
                    return String(row.id || row.node_id || "") === currentId;
                }) || state.currentMapRow;
                renderMapForContact(freshRow);

                if (state.currentMapMode === "path")
                {
                    await showNodeAdvertPath(freshRow);
                }

                return;
            }

            if (state.view === "info")
            {
                renderInfo();
                return;
            }

            await loadNodes();
            renderContacts();
        }
        catch (error)
        {
            showError(error.message || tr("error.unknown", "Unbekannter Fehler"));
        }
        finally
        {
            setBusy(false);
        }
    }

    async function switchView(view)
    {
        if (view === "contacts")
        {
            await loadNodes();
            renderContacts();
            return;
        }

        if (view === "channels")
        {
            await loadChannels();
            renderChannels();
            return;
        }

        renderInfo();
    }

    function bindEvents()
    {
        el.refreshButton.addEventListener("click", refreshCurrentView);
        el.backButton.addEventListener("click", function()
        {
            if (state.view === "messagePathMap" && state.currentChat)
            {
                openChat(state.currentChat).catch(function(error)
                {
                    showError(error.message || tr("error.unknown", "Unbekannter Fehler"));
                });
                return;
            }

            switchView("contacts").catch(function(error)
            {
                showError(error.message || tr("error.unknown", "Unbekannter Fehler"));
            });
        });

        el.sendButton.addEventListener("click", sendMessage);

        el.input.addEventListener("keydown", function(event)
        {
            if (event.key === "Enter" && !event.shiftKey)
            {
                event.preventDefault();
                sendMessage();
            }
        });


        el.setupCancelButton?.addEventListener("click", closeSetupDialog);
        el.setupApplyButton?.addEventListener("click", async function()
        {
            try
            {
                await applyCompanionSetup();
                closeSetupDialog();
            }
            catch (error)
            {
                showSetupError(error.message || tr("setup.error.apply_failed", "Setup fehlgeschlagen."));
            }
        });

        el.setupModal?.addEventListener("click", function(event)
        {
            if (event.target === el.setupModal)
            {
                closeSetupDialog();
            }
        });

        el.discoverRepeatButton?.addEventListener("click", handleDiscoverStartClick);
        el.discoverCloseButton?.addEventListener("click", function()
        {
            closeDiscoverDialog();
        });

        el.discoverModal?.addEventListener("click", function(event)
        {
            if (event.target === el.discoverModal)
            {
                closeDiscoverDialog();
            }
        });

        el.channelCancelButton?.addEventListener("click", closeMobileChannelDialog);
        el.channelConfirmButton?.addEventListener("click", function()
        {
            handleMobileChannelConfirm().catch(function(error)
            {
                setChannelModalError(error.message || tr("error.unknown", "Unbekannter Fehler"));
            });
        });
        el.channelModal?.addEventListener("click", function(event)
        {
            if (event.target === el.channelModal)
            {
                closeMobileChannelDialog();
            }
        });

        el.languageCloseButton?.addEventListener("click", closeLanguageDialog);
        el.languageModal?.addEventListener("click", function(event)
        {
            if (event.target === el.languageModal)
            {
                closeLanguageDialog();
            }
        });

        el.languageSelect?.addEventListener("change", function()
        {
            if (typeof global.setLanguage === "function")
            {
                global.setLanguage(el.languageSelect.value);
                refreshCurrentViewAfterLanguageChange();
            }
        });

        el.nav.addEventListener("click", function(event)
        {
            const button = event.target.closest("[data-view]");

            if (!button)
            {
                return;
            }

            switchView(button.dataset.view).catch(function(error)
            {
                showError(error.message || tr("error.unknown", "Unbekannter Fehler"));
            });
        });
    }

    async function init()
    {
        if (typeof Api.loadNodes !== "function" || typeof Api.loadMessages !== "function")
        {
            showError(tr("system.api_missing", "MeshCoreApi wurde nicht geladen."));
            return;
        }

        bindEvents();
        await loadNodes();
        renderContacts();

        state.radioStatusRefreshTimer = setInterval(function()
        {
            refreshMobileRadioStatus();
        }, 3000);
    }

    async function sendMobileFloodAdvert(button)
    {
        if (button)
        {
            button.disabled = true;
        }

        try
        {
            const result = await Api.sendFloodAdvert();

            if (!result.success)
            {
                throw new Error(result.error || "Unbekannter Fehler");
            }
        }
        catch (error)
        {
            alert("Advert senden fehlgeschlagen: " + (error.message || "Unbekannter Fehler"));
        }
        finally
        {
            if (button)
            {
                button.disabled = false;
            }
        }
    }

    init().catch(function(error)
    {
        showError(error.message || tr("error.unknown", "Unbekannter Fehler"));
    });
})(window);
