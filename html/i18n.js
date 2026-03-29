const I18N =
{
    de:
    {
        "html.lang": "de",
        "document.title": "MeshCore Dashboard",
        "page.title": "MeshCore Web-Dashboard",
        "page.subtitle": "by DJ0ABR",

        "panel.contacts_channels": "Kontakte/Kanäle",
        "panel.sort_hint": "sortierbar per Klick auf den Spaltenkopf",
        "panel.messages": "Messages",

        "toolbar.type": "Typ:",
        "toolbar.filter.all": "Alle",
        "toolbar.filter.chat": "Chat",
        "toolbar.filter.repeater": "Repeater",
        "toolbar.filter.room": "Raum",
        "toolbar.filter.sensor": "Sensor",
        "toolbar.all_map": "Karte Alle",
        "toolbar.advert": "Advert",

        "tabs.contacts": "Kontakte",
        "tabs.channels": "Kanäle",

        "chat.title.default": "Chat",
        "chat.input_placeholder": "Nachricht eingeben und Enter drücken ...",
        "chat.send": "Senden",
        "chat.empty": "Keine Messages für diesen Eintrag gefunden.",
        "chat.loading": "Lade Messages ...",
        "chat.error_loading": "Fehler beim Laden der Messages: {message}",
        "chat.refresh_failed": "Chat-Refresh fehlgeschlagen: {message}",
        "chat.status.pending": "Ausstehend",
        "chat.you": "Du",
        "chat.snr": "SNR",
        "chat.path": "Pfad",
        "chat.send_error": "Fehler beim Senden: {message}",

        "map.empty.default": "Linksklick auf Chat, Raum oder Kanal = Verlauf, Rechtsklick = Karte.",
        "map.empty.no_nodes": "Keine Nodes mit Position vorhanden.",
        "map.no_position_for_node": "keine Positionsdaten verfügbar.",

        "room_password.title": "Passwort für Raum",
        "room_password.title_with_name": "Passwort für Raum {name}",
        "room_password.label": "Raum Passwort",
        "room_password.subtitle.required": "Das Backend meldet: Raum Password erforderlich",
        "room_password.cancel": "Abbrechen",
        "room_password.save": "Speichern",
        "room_password.error.empty": "Bitte ein Passwort eingeben.",
        "room_password.error.save_failed": "Speichern fehlgeschlagen.",

        "channel.modal.base_title": "Kanal",
        "channel.name": "Kanalname",
        "channel.secret": "Secret Key",
        "channel.generated_secret": "Generierter Secret Key",
        "channel.qr_code": "QR-Code",
        "channel.ok": "OK",
        "channel.cancel": "Abbrechen",
        "channel.none_selected": "Kein Kanal ausgewählt.",
        "channel.invalid_action": "Ungültige Aktion.",
        "channel.fallback_name": "Kanal {idx}",
        "channel.meta.default": "Standard",
        "channel.meta.observed": "beobachtet",
        "channel.meta.disabled": "deaktiviert",
        "channel.send.disabled": "Dieser Kanal ist deaktiviert.",
        "channel.send.observed_no_context": "Für diesen beobachteten Kanal ist kein lokaler Sendekontext konfiguriert.",

        "channel.action.create_private.title": "Privaten Kanal anlegen",
        "channel.action.create_private.subtitle": "Neuen privaten Kanal anlegen. Der Secret Key wird anschließend angezeigt.",
        "channel.action.create_private.confirm": "Erstellen",
        "channel.action.create_private.done_subtitle": "Kanal wurde erstellt. Diesen Secret Key jetzt mit den anderen Teilnehmern teilen.",

        "channel.action.join_private.title": "Privatem Kanal beitreten",
        "channel.action.join_private.subtitle": "Kanalname und Secret Key eingeben.",
        "channel.action.join_private.confirm": "Beitreten",

        "channel.action.join_public.title": "Öffentlichem Kanal beitreten",
        "channel.action.join_public.subtitle": "Öffentlichem Kanal über den Namen beitreten.",
        "channel.action.join_public.confirm": "Beitreten",

        "channel.action.join_hashtag.title": "Hashtag-Kanal beitreten",
        "channel.action.join_hashtag.subtitle": "Hashtag-Kanal eingeben, z. B. #drones.",
        "channel.action.join_hashtag.confirm": "Beitreten",

        "channel.action.remove.title": "Kanal entfernen",
        "channel.action.remove.subtitle": "Kanal wirklich entfernen: {name} (IDX {idx})",
        "channel.action.remove.none_selected": "Kein Kanal ausgewählt.",
        "channel.action.remove.confirm": "Entfernen",

        "channel.button.create_private": "Privat erstellen",
        "channel.button.join_private": "Privat beitreten",
        "channel.button.join_public": "Öffentlich beitreten",
        "channel.button.join_hashtag": "Hashtag beitreten",
        "channel.button.remove": "Entfernen",

        "channel.type.chat": "Chat",
        "channel.type.room": "Raum",
        "channel.type.channel": "Kanal",
        "channel.kind.dm": "dm",
        "channel.kind.room": "room",
        "channel.kind.channel": "channel",

        "channels.none": "Keine Kanäle vorhanden",

        "discover.title": "Repeater finden",
        "discover.status.label": "Status",
        "discover.status.unknown": "Unbekannt",
        "discover.job.label": "Letzter Job",
        "discover.job.none": "Noch keine Suche gestartet.",
        "discover.results.label": "Gefundene Repeater",
        "discover.results.none": "Noch keine Ergebnisse.",
        "discover.button.start": "START",
        "discover.button.close": "Schließen",
        "discover.error.start_failed": "Suche konnte nicht gestartet werden.",
        "discover.error.status_failed": "Such-Status konnte nicht geladen werden.",
        "discover.status.queued": "Warteschlange",
        "discover.status.running": "läuft",
        "discover.status.done": "fertig",
        "discover.status.failed": "Fehler",
        "discover.status.skipped": "übersprungen",
        "discover.status.unknown_fallback": "unbekannt",
        "discover.updated": "Update",
        "discover.results_count": "Treffer",
        "discover.finished": "beendet",
        "discover.started": "gestartet",
        "discover.created": "angelegt",
        "discover.error_label": "Fehler",

        "setup.title": "Companion Setup",
        "setup.name": "Name",
        "setup.latitude": "Breitegrad",
        "setup.longitude": "Längengrad",
        "setup.fixed_radio": "Feste Funkparameter",
        "setup.apply": "Anwenden",
        "setup.cancel": "Abbrechen",
        "setup.error.name_missing": "Name fehlt.",
        "setup.error.latitude_invalid": "Breitegrad ist ungültig.",
        "setup.error.longitude_invalid": "Längengrad ist ungültig.",
        "setup.error.load_failed": "Setup-Werte konnten nicht geladen werden.",
        "setup.error.apply_failed": "Setup fehlgeschlagen.",

        "type.chat": "Chat",
        "type.repeater": "Repeater",
        "type.room": "Raum",
        "type.sensor": "Sensor",
        "type.unknown": "Unbekannt",

        "table.placeholder.no_nodes": "Keine Nodes gefunden.",
        "table.column.type": "Typ",
        "table.column.name": "Name",
        "table.column.last": "Zuletzt",

        "counter.channels_observed": "{count} Kanäle ({observed} beobachtet)",
        "counter.channels": "{count} Kanäle",
        "counter.nodes": "{count} Nodes",

        "status.error": "Fehler",

        "common.done": "Fertig",
        "common.cancel": "Abbrechen",
        "common.save": "Speichern",

        "error.unknown": "Unbekannter Fehler",
        "error.qr_library_missing": "QR-Code Bibliothek nicht geladen.",
        "error.loading": "Fehler beim Laden: {message}",
        "error.invalid_json": "keine gültige JSON-Antwort",
        "error.http": "HTTP Fehler",
        "error.generic": "Fehler",
        "room_password.input_placeholder": "Passwort eingeben ..."
    },

    en:
    {
        "html.lang": "en",
        "document.title": "MeshCore Dashboard",
        "page.title": "MeshCore Web Dashboard",
        "page.subtitle": "by DJ0ABR",

        "panel.contacts_channels": "Contacts/Channels",
        "panel.sort_hint": "sortable by clicking the column header",
        "panel.messages": "Messages",

        "toolbar.type": "Type:",
        "toolbar.filter.all": "All",
        "toolbar.filter.chat": "Chat",
        "toolbar.filter.repeater": "Repeater",
        "toolbar.filter.room": "Room",
        "toolbar.filter.sensor": "Sensor",
        "toolbar.all_map": "All Map",
        "toolbar.advert": "Advert",

        "tabs.contacts": "Contacts",
        "tabs.channels": "Channels",

        "chat.title.default": "Chat",
        "chat.input_placeholder": "Enter message and press Enter ...",
        "chat.send": "Send",
        "chat.empty": "No messages found for this entry.",
        "chat.loading": "Loading messages ...",
        "chat.error_loading": "Error loading messages: {message}",
        "chat.refresh_failed": "Chat refresh failed: {message}",
        "chat.status.pending": "Pending",
        "chat.you": "You",
        "chat.snr": "SNR",
        "chat.path": "Path",
        "chat.send_error": "Error sending message: {message}",

        "map.empty.default": "Left click on chat, room or channel = history, right click = map.",
        "map.empty.no_nodes": "No nodes with position available.",
        "map.no_position_for_node": "no position data available.",

        "room_password.title": "Password for room",
        "room_password.title_with_name": "Password for room {name}",
        "room_password.label": "Room password",
        "room_password.subtitle.required": "The backend reports: room password required",
        "room_password.cancel": "Cancel",
        "room_password.save": "Save",
        "room_password.error.empty": "Please enter a password.",
        "room_password.error.save_failed": "Saving failed.",

        "channel.modal.base_title": "Channel",
        "channel.name": "Channel name",
        "channel.secret": "Secret key",
        "channel.generated_secret": "Generated secret key",
        "channel.qr_code": "QR code",
        "channel.ok": "OK",
        "channel.cancel": "Cancel",
        "channel.none_selected": "No channel selected.",
        "channel.invalid_action": "Invalid action.",
        "channel.fallback_name": "Channel {idx}",
        "channel.meta.default": "default",
        "channel.meta.observed": "observed",
        "channel.meta.disabled": "disabled",
        "channel.send.disabled": "This channel is disabled.",
        "channel.send.observed_no_context": "No local sending context is configured for this observed channel.",

        "channel.action.create_private.title": "Create private channel",
        "channel.action.create_private.subtitle": "Create a new private channel. The secret key will be shown afterwards.",
        "channel.action.create_private.confirm": "Create",
        "channel.action.create_private.done_subtitle": "Channel created. Share this secret key with the other participants now.",

        "channel.action.join_private.title": "Join private channel",
        "channel.action.join_private.subtitle": "Enter channel name and secret key.",
        "channel.action.join_private.confirm": "Join",

        "channel.action.join_public.title": "Join public channel",
        "channel.action.join_public.subtitle": "Join a public channel by name.",
        "channel.action.join_public.confirm": "Join",

        "channel.action.join_hashtag.title": "Join hashtag channel",
        "channel.action.join_hashtag.subtitle": "Enter a hashtag channel, e.g. #drones.",
        "channel.action.join_hashtag.confirm": "Join",

        "channel.action.remove.title": "Remove channel",
        "channel.action.remove.subtitle": "Really remove channel: {name} (IDX {idx})",
        "channel.action.remove.none_selected": "No channel selected.",
        "channel.action.remove.confirm": "Remove",

        "channel.button.create_private": "Create Private",
        "channel.button.join_private": "Join Private",
        "channel.button.join_public": "Join Public",
        "channel.button.join_hashtag": "Join Hashtag",
        "channel.button.remove": "Remove",

        "channel.type.chat": "Chat",
        "channel.type.room": "Room",
        "channel.type.channel": "Channel",
        "channel.kind.dm": "dm",
        "channel.kind.room": "room",
        "channel.kind.channel": "channel",

        "channels.none": "No channels available",

        "discover.title": "Repeater Discovery",
        "discover.status.label": "Status",
        "discover.status.unknown": "Unknown",
        "discover.job.label": "Last job",
        "discover.job.none": "No discovery has been started yet.",
        "discover.results.label": "Found repeaters",
        "discover.results.none": "No results yet.",
        "discover.button.start": "START",
        "discover.button.close": "Close",
        "discover.error.start_failed": "Failed to start discovery.",
        "discover.error.status_failed": "Failed to load discovery status.",
        "discover.status.queued": "queued",
        "discover.status.running": "running",
        "discover.status.done": "done",
        "discover.status.failed": "failed",
        "discover.status.skipped": "skipped",
        "discover.status.unknown_fallback": "unknown",
        "discover.updated": "Updated",
        "discover.results_count": "Results",
        "discover.finished": "finished",
        "discover.started": "started",
        "discover.created": "created",
        "discover.error_label": "Error",

        "setup.title": "Companion Setup",
        "setup.name": "Name",
        "setup.latitude": "Latitude",
        "setup.longitude": "Longitude",
        "setup.fixed_radio": "Fixed radio parameters",
        "setup.apply": "Apply",
        "setup.cancel": "Cancel",
        "setup.error.name_missing": "Name is missing.",
        "setup.error.latitude_invalid": "Latitude is invalid.",
        "setup.error.longitude_invalid": "Longitude is invalid.",
        "setup.error.load_failed": "Failed to load setup values.",
        "setup.error.apply_failed": "Failed to apply setup.",

        "type.chat": "Chat",
        "type.repeater": "Repeater",
        "type.room": "Room",
        "type.sensor": "Sensor",
        "type.unknown": "Unknown",

        "table.placeholder.no_nodes": "No nodes found.",
        "table.column.type": "Type",
        "table.column.name": "Name",
        "table.column.last": "Last",

        "counter.channels_observed": "{count} channels ({observed} observed)",
        "counter.channels": "{count} channels",
        "counter.nodes": "{count} nodes",

        "status.error": "Error",

        "common.done": "Done",
        "common.cancel": "Cancel",
        "common.save": "Save",

        "error.unknown": "Unknown error",
        "error.qr_library_missing": "QR code library not loaded.",
        "error.loading": "Error loading data: {message}",
        "error.invalid_json": "invalid JSON response",
        "error.http": "HTTP error",
        "error.generic": "Error",
        "room_password.input_placeholder": "Enter password ..."
    },

    es:
    {
        "html.lang": "es",
        "document.title": "MeshCore Dashboard",
        "page.title": "Panel web de MeshCore",
        "page.subtitle": "by DJ0ABR",

        "panel.contacts_channels": "Contactos/Canales",
        "panel.sort_hint": "ordenable haciendo clic en el encabezado de la columna",
        "panel.messages": "Mensajes",

        "toolbar.type": "Tipo:",
        "toolbar.filter.all": "Todos",
        "toolbar.filter.chat": "Chat",
        "toolbar.filter.repeater": "Repetidor",
        "toolbar.filter.room": "Sala",
        "toolbar.filter.sensor": "Sensor",
        "toolbar.all_map": "Mapa completo",
        "toolbar.advert": "Advert",

        "tabs.contacts": "Contactos",
        "tabs.channels": "Canales",

        "chat.title.default": "Chat",
        "chat.input_placeholder": "Escribe un mensaje y pulsa Enter ...",
        "chat.send": "Enviar",
        "chat.empty": "No se encontraron mensajes para esta entrada.",
        "chat.loading": "Cargando mensajes ...",
        "chat.error_loading": "Error al cargar los mensajes: {message}",
        "chat.refresh_failed": "La actualización del chat falló: {message}",
        "chat.status.pending": "Pendiente",
        "chat.you": "Tú",
        "chat.snr": "SNR",
        "chat.path": "Ruta",
        "chat.send_error": "Error al enviar el mensaje: {message}",

        "map.empty.default": "Clic izquierdo en chat, sala o canal = historial, clic derecho = mapa.",
        "map.empty.no_nodes": "No hay nodos con posición disponible.",
        "map.no_position_for_node": "no hay datos de posición disponibles.",

        "room_password.title": "Contraseña para la sala",
        "room_password.title_with_name": "Contraseña para la sala {name}",
        "room_password.label": "Contraseña de la sala",
        "room_password.subtitle.required": "El backend informa: se requiere contraseña de sala",
        "room_password.cancel": "Cancelar",
        "room_password.save": "Guardar",
        "room_password.error.empty": "Por favor, introduce una contraseña.",
        "room_password.error.save_failed": "No se pudo guardar.",

        "channel.modal.base_title": "Canal",
        "channel.name": "Nombre del canal",
        "channel.secret": "Clave secreta",
        "channel.generated_secret": "Clave secreta generada",
        "channel.qr_code": "Código QR",
        "channel.ok": "OK",
        "channel.cancel": "Cancelar",
        "channel.none_selected": "No hay ningún canal seleccionado.",
        "channel.invalid_action": "Acción no válida.",
        "channel.fallback_name": "Canal {idx}",
        "channel.meta.default": "predeterminado",
        "channel.meta.observed": "observado",
        "channel.meta.disabled": "desactivado",
        "channel.send.disabled": "Este canal está desactivado.",
        "channel.send.observed_no_context": "No hay un contexto local de envío configurado para este canal observado.",

        "channel.action.create_private.title": "Crear canal privado",
        "channel.action.create_private.subtitle": "Crear un nuevo canal privado. La clave secreta se mostrará después.",
        "channel.action.create_private.confirm": "Crear",
        "channel.action.create_private.done_subtitle": "Canal creado. Comparte ahora esta clave secreta con los demás participantes.",

        "channel.action.join_private.title": "Unirse a un canal privado",
        "channel.action.join_private.subtitle": "Introduce el nombre del canal y la clave secreta.",
        "channel.action.join_private.confirm": "Unirse",

        "channel.action.join_public.title": "Unirse a un canal público",
        "channel.action.join_public.subtitle": "Unirse a un canal público por nombre.",
        "channel.action.join_public.confirm": "Unirse",

        "channel.action.join_hashtag.title": "Unirse a un canal hashtag",
        "channel.action.join_hashtag.subtitle": "Introduce un canal hashtag, por ejemplo #drones.",
        "channel.action.join_hashtag.confirm": "Unirse",

        "channel.action.remove.title": "Eliminar canal",
        "channel.action.remove.subtitle": "¿Eliminar realmente el canal: {name} (IDX {idx})?",
        "channel.action.remove.none_selected": "No hay ningún canal seleccionado.",
        "channel.action.remove.confirm": "Eliminar",

        "channel.button.create_private": "Crear privado",
        "channel.button.join_private": "Unirse privado",
        "channel.button.join_public": "Unirse público",
        "channel.button.join_hashtag": "Unirse hashtag",
        "channel.button.remove": "Eliminar",

        "channel.type.chat": "Chat",
        "channel.type.room": "Sala",
        "channel.type.channel": "Canal",
        "channel.kind.dm": "dm",
        "channel.kind.room": "room",
        "channel.kind.channel": "channel",

        "channels.none": "No hay canales disponibles",

        "discover.title": "Buscar repetidores",
        "discover.status.label": "Estado",
        "discover.status.unknown": "Desconocido",
        "discover.job.label": "Último trabajo",
        "discover.job.none": "Aún no se ha iniciado ninguna búsqueda.",
        "discover.results.label": "Repetidores encontrados",
        "discover.results.none": "Aún no hay resultados.",
        "discover.button.start": "INICIAR",
        "discover.button.close": "Cerrar",
        "discover.error.start_failed": "No se pudo iniciar la búsqueda.",
        "discover.error.status_failed": "No se pudo cargar el estado de la búsqueda.",
        "discover.status.queued": "en cola",
        "discover.status.running": "en ejecución",
        "discover.status.done": "terminado",
        "discover.status.failed": "error",
        "discover.status.skipped": "omitido",
        "discover.status.unknown_fallback": "desconocido",
        "discover.updated": "Actualizado",
        "discover.results_count": "Resultados",
        "discover.finished": "finalizado",
        "discover.started": "iniciado",
        "discover.created": "creado",
        "discover.error_label": "Error",

        "setup.title": "Configuración Companion",
        "setup.name": "Nombre",
        "setup.latitude": "Latitud",
        "setup.longitude": "Longitud",
        "setup.fixed_radio": "Parámetros de radio fijos",
        "setup.apply": "Aplicar",
        "setup.cancel": "Cancelar",
        "setup.error.name_missing": "Falta el nombre.",
        "setup.error.latitude_invalid": "La latitud no es válida.",
        "setup.error.longitude_invalid": "La longitud no es válida.",
        "setup.error.load_failed": "No se pudieron cargar los valores de configuración.",
        "setup.error.apply_failed": "La configuración falló.",

        "type.chat": "Chat",
        "type.repeater": "Repetidor",
        "type.room": "Sala",
        "type.sensor": "Sensor",
        "type.unknown": "Desconocido",

        "table.placeholder.no_nodes": "No se encontraron nodos.",
        "table.column.type": "Tipo",
        "table.column.name": "Nombre",
        "table.column.last": "Último",

        "counter.channels_observed": "{count} canales ({observed} observados)",
        "counter.channels": "{count} canales",
        "counter.nodes": "{count} nodos",

        "status.error": "Error",

        "common.done": "Hecho",
        "common.cancel": "Cancelar",
        "common.save": "Guardar",

        "error.unknown": "Error desconocido",
        "error.qr_library_missing": "La biblioteca de código QR no está cargada.",
        "error.loading": "Error al cargar: {message}",
        "error.invalid_json": "respuesta JSON no válida",
        "error.http": "Error HTTP",
        "error.generic": "Error",
        "room_password.input_placeholder": "Introduce la contraseña ..."
    },

    fr:
    {
        "html.lang": "fr",
        "document.title": "MeshCore Dashboard",
        "page.title": "Tableau de bord web MeshCore",
        "page.subtitle": "by DJ0ABR",

        "panel.contacts_channels": "Contacts/Canaux",
        "panel.sort_hint": "triable en cliquant sur l’en-tête de colonne",
        "panel.messages": "Messages",

        "toolbar.type": "Type :",
        "toolbar.filter.all": "Tous",
        "toolbar.filter.chat": "Chat",
        "toolbar.filter.repeater": "Répéteur",
        "toolbar.filter.room": "Salle",
        "toolbar.filter.sensor": "Capteur",
        "toolbar.all_map": "Carte complète",
        "toolbar.advert": "Advert",

        "tabs.contacts": "Contacts",
        "tabs.channels": "Canaux",

        "chat.title.default": "Chat",
        "chat.input_placeholder": "Saisissez un message et appuyez sur Entrée ...",
        "chat.send": "Envoyer",
        "chat.empty": "Aucun message trouvé pour cette entrée.",
        "chat.loading": "Chargement des messages ...",
        "chat.error_loading": "Erreur lors du chargement des messages : {message}",
        "chat.refresh_failed": "L’actualisation du chat a échoué : {message}",
        "chat.status.pending": "En attente",
        "chat.you": "Vous",
        "chat.snr": "SNR",
        "chat.path": "Chemin",
        "chat.send_error": "Erreur lors de l’envoi du message : {message}",

        "map.empty.default": "Clic gauche sur chat, salle ou canal = historique, clic droit = carte.",
        "map.empty.no_nodes": "Aucun nœud avec position disponible.",
        "map.no_position_for_node": "aucune donnée de position disponible.",

        "room_password.title": "Mot de passe pour la salle",
        "room_password.title_with_name": "Mot de passe pour la salle {name}",
        "room_password.label": "Mot de passe de la salle",
        "room_password.subtitle.required": "Le backend indique : mot de passe de salle requis",
        "room_password.cancel": "Annuler",
        "room_password.save": "Enregistrer",
        "room_password.error.empty": "Veuillez saisir un mot de passe.",
        "room_password.error.save_failed": "Échec de l’enregistrement.",

        "channel.modal.base_title": "Canal",
        "channel.name": "Nom du canal",
        "channel.secret": "Clé secrète",
        "channel.generated_secret": "Clé secrète générée",
        "channel.qr_code": "Code QR",
        "channel.ok": "OK",
        "channel.cancel": "Annuler",
        "channel.none_selected": "Aucun canal sélectionné.",
        "channel.invalid_action": "Action invalide.",
        "channel.fallback_name": "Canal {idx}",
        "channel.meta.default": "par défaut",
        "channel.meta.observed": "observé",
        "channel.meta.disabled": "désactivé",
        "channel.send.disabled": "Ce canal est désactivé.",
        "channel.send.observed_no_context": "Aucun contexte local d’envoi n’est configuré pour ce canal observé.",

        "channel.action.create_private.title": "Créer un canal privé",
        "channel.action.create_private.subtitle": "Créer un nouveau canal privé. La clé secrète sera affichée ensuite.",
        "channel.action.create_private.confirm": "Créer",
        "channel.action.create_private.done_subtitle": "Canal créé. Partagez maintenant cette clé secrète avec les autres participants.",

        "channel.action.join_private.title": "Rejoindre un canal privé",
        "channel.action.join_private.subtitle": "Saisissez le nom du canal et la clé secrète.",
        "channel.action.join_private.confirm": "Rejoindre",

        "channel.action.join_public.title": "Rejoindre un canal public",
        "channel.action.join_public.subtitle": "Rejoindre un canal public par son nom.",
        "channel.action.join_public.confirm": "Rejoindre",

        "channel.action.join_hashtag.title": "Rejoindre un canal hashtag",
        "channel.action.join_hashtag.subtitle": "Saisissez un canal hashtag, par ex. #drones.",
        "channel.action.join_hashtag.confirm": "Rejoindre",

        "channel.action.remove.title": "Supprimer le canal",
        "channel.action.remove.subtitle": "Supprimer vraiment le canal : {name} (IDX {idx})",
        "channel.action.remove.none_selected": "Aucun canal sélectionné.",
        "channel.action.remove.confirm": "Supprimer",

        "channel.button.create_private": "Créer privé",
        "channel.button.join_private": "Rejoindre privé",
        "channel.button.join_public": "Rejoindre public",
        "channel.button.join_hashtag": "Rejoindre hashtag",
        "channel.button.remove": "Supprimer",

        "channel.type.chat": "Chat",
        "channel.type.room": "Salle",
        "channel.type.channel": "Canal",
        "channel.kind.dm": "dm",
        "channel.kind.room": "room",
        "channel.kind.channel": "channel",

        "channels.none": "Aucun canal disponible",

        "discover.title": "Recherche de répéteurs",
        "discover.status.label": "Statut",
        "discover.status.unknown": "Inconnu",
        "discover.job.label": "Dernier job",
        "discover.job.none": "Aucune recherche n’a encore été lancée.",
        "discover.results.label": "Répéteurs trouvés",
        "discover.results.none": "Aucun résultat pour le moment.",
        "discover.button.start": "DÉMARRER",
        "discover.button.close": "Fermer",
        "discover.error.start_failed": "Impossible de démarrer la recherche.",
        "discover.error.status_failed": "Impossible de charger le statut de la recherche.",
        "discover.status.queued": "en file d’attente",
        "discover.status.running": "en cours",
        "discover.status.done": "terminé",
        "discover.status.failed": "erreur",
        "discover.status.skipped": "ignoré",
        "discover.status.unknown_fallback": "inconnu",
        "discover.updated": "Mis à jour",
        "discover.results_count": "Résultats",
        "discover.finished": "terminé",
        "discover.started": "démarré",
        "discover.created": "créé",
        "discover.error_label": "Erreur",

        "setup.title": "Configuration Companion",
        "setup.name": "Nom",
        "setup.latitude": "Latitude",
        "setup.longitude": "Longitude",
        "setup.fixed_radio": "Paramètres radio fixes",
        "setup.apply": "Appliquer",
        "setup.cancel": "Annuler",
        "setup.error.name_missing": "Le nom manque.",
        "setup.error.latitude_invalid": "La latitude est invalide.",
        "setup.error.longitude_invalid": "La longitude est invalide.",
        "setup.error.load_failed": "Impossible de charger les valeurs de configuration.",
        "setup.error.apply_failed": "Échec de la configuration.",

        "type.chat": "Chat",
        "type.repeater": "Répéteur",
        "type.room": "Salle",
        "type.sensor": "Capteur",
        "type.unknown": "Inconnu",

        "table.placeholder.no_nodes": "Aucun nœud trouvé.",
        "table.column.type": "Type",
        "table.column.name": "Nom",
        "table.column.last": "Dernier",

        "counter.channels_observed": "{count} canaux ({observed} observés)",
        "counter.channels": "{count} canaux",
        "counter.nodes": "{count} nœuds",

        "status.error": "Erreur",

        "common.done": "Terminé",
        "common.cancel": "Annuler",
        "common.save": "Enregistrer",

        "error.unknown": "Erreur inconnue",
        "error.qr_library_missing": "La bibliothèque de code QR n’est pas chargée.",
        "error.loading": "Erreur lors du chargement : {message}",
        "error.invalid_json": "réponse JSON invalide",
        "error.http": "Erreur HTTP",
        "error.generic": "Erreur",
        "room_password.input_placeholder": "Saisissez le mot de passe ..."
    },

    it:
    {
        "html.lang": "it",
        "document.title": "MeshCore Dashboard",
        "page.title": "Dashboard web MeshCore",
        "page.subtitle": "by DJ0ABR",

        "panel.contacts_channels": "Contatti/Canali",
        "panel.sort_hint": "ordinabile cliccando sull’intestazione della colonna",
        "panel.messages": "Messaggi",

        "toolbar.type": "Tipo:",
        "toolbar.filter.all": "Tutti",
        "toolbar.filter.chat": "Chat",
        "toolbar.filter.repeater": "Ripetitore",
        "toolbar.filter.room": "Stanza",
        "toolbar.filter.sensor": "Sensore",
        "toolbar.all_map": "Mappa completa",
        "toolbar.advert": "Advert",

        "tabs.contacts": "Contatti",
        "tabs.channels": "Canali",

        "chat.title.default": "Chat",
        "chat.input_placeholder": "Inserisci un messaggio e premi Invio ...",
        "chat.send": "Invia",
        "chat.empty": "Nessun messaggio trovato per questa voce.",
        "chat.loading": "Caricamento messaggi ...",
        "chat.error_loading": "Errore durante il caricamento dei messaggi: {message}",
        "chat.refresh_failed": "Aggiornamento chat non riuscito: {message}",
        "chat.status.pending": "In attesa",
        "chat.you": "Tu",
        "chat.snr": "SNR",
        "chat.path": "Percorso",
        "chat.send_error": "Errore durante l’invio del messaggio: {message}",

        "map.empty.default": "Clic sinistro su chat, stanza o canale = cronologia, clic destro = mappa.",
        "map.empty.no_nodes": "Nessun nodo con posizione disponibile.",
        "map.no_position_for_node": "nessun dato di posizione disponibile.",

        "room_password.title": "Password per la stanza",
        "room_password.title_with_name": "Password per la stanza {name}",
        "room_password.label": "Password della stanza",
        "room_password.subtitle.required": "Il backend segnala: password della stanza richiesta",
        "room_password.cancel": "Annulla",
        "room_password.save": "Salva",
        "room_password.error.empty": "Inserisci una password.",
        "room_password.error.save_failed": "Salvataggio non riuscito.",

        "channel.modal.base_title": "Canale",
        "channel.name": "Nome canale",
        "channel.secret": "Chiave segreta",
        "channel.generated_secret": "Chiave segreta generata",
        "channel.qr_code": "Codice QR",
        "channel.ok": "OK",
        "channel.cancel": "Annulla",
        "channel.none_selected": "Nessun canale selezionato.",
        "channel.invalid_action": "Azione non valida.",
        "channel.fallback_name": "Canale {idx}",
        "channel.meta.default": "predefinito",
        "channel.meta.observed": "osservato",
        "channel.meta.disabled": "disattivato",
        "channel.send.disabled": "Questo canale è disattivato.",
        "channel.send.observed_no_context": "Nessun contesto locale di invio è configurato per questo canale osservato.",

        "channel.action.create_private.title": "Crea canale privato",
        "channel.action.create_private.subtitle": "Crea un nuovo canale privato. La chiave segreta verrà mostrata successivamente.",
        "channel.action.create_private.confirm": "Crea",
        "channel.action.create_private.done_subtitle": "Canale creato. Condividi ora questa chiave segreta con gli altri partecipanti.",

        "channel.action.join_private.title": "Unisciti a un canale privato",
        "channel.action.join_private.subtitle": "Inserisci nome canale e chiave segreta.",
        "channel.action.join_private.confirm": "Unisciti",

        "channel.action.join_public.title": "Unisciti a un canale pubblico",
        "channel.action.join_public.subtitle": "Unisciti a un canale pubblico tramite nome.",
        "channel.action.join_public.confirm": "Unisciti",

        "channel.action.join_hashtag.title": "Unisciti a un canale hashtag",
        "channel.action.join_hashtag.subtitle": "Inserisci un canale hashtag, ad esempio #drones.",
        "channel.action.join_hashtag.confirm": "Unisciti",

        "channel.action.remove.title": "Rimuovi canale",
        "channel.action.remove.subtitle": "Rimuovere davvero il canale: {name} (IDX {idx})",
        "channel.action.remove.none_selected": "Nessun canale selezionato.",
        "channel.action.remove.confirm": "Rimuovi",

        "channel.button.create_private": "Crea privato",
        "channel.button.join_private": "Unisciti privato",
        "channel.button.join_public": "Unisciti pubblico",
        "channel.button.join_hashtag": "Unisciti hashtag",
        "channel.button.remove": "Rimuovi",

        "channel.type.chat": "Chat",
        "channel.type.room": "Stanza",
        "channel.type.channel": "Canale",
        "channel.kind.dm": "dm",
        "channel.kind.room": "room",
        "channel.kind.channel": "channel",

        "channels.none": "Nessun canale disponibile",

        "discover.title": "Ricerca ripetitori",
        "discover.status.label": "Stato",
        "discover.status.unknown": "Sconosciuto",
        "discover.job.label": "Ultimo job",
        "discover.job.none": "Nessuna ricerca è stata ancora avviata.",
        "discover.results.label": "Ripetitori trovati",
        "discover.results.none": "Nessun risultato al momento.",
        "discover.button.start": "AVVIA",
        "discover.button.close": "Chiudi",
        "discover.error.start_failed": "Impossibile avviare la ricerca.",
        "discover.error.status_failed": "Impossibile caricare lo stato della ricerca.",
        "discover.status.queued": "in coda",
        "discover.status.running": "in esecuzione",
        "discover.status.done": "completato",
        "discover.status.failed": "errore",
        "discover.status.skipped": "saltato",
        "discover.status.unknown_fallback": "sconosciuto",
        "discover.updated": "Aggiornato",
        "discover.results_count": "Risultati",
        "discover.finished": "terminato",
        "discover.started": "avviato",
        "discover.created": "creato",
        "discover.error_label": "Errore",

        "setup.title": "Configurazione Companion",
        "setup.name": "Nome",
        "setup.latitude": "Latitudine",
        "setup.longitude": "Longitudine",
        "setup.fixed_radio": "Parametri radio fissi",
        "setup.apply": "Applica",
        "setup.cancel": "Annulla",
        "setup.error.name_missing": "Manca il nome.",
        "setup.error.latitude_invalid": "La latitudine non è valida.",
        "setup.error.longitude_invalid": "La longitudine non è valida.",
        "setup.error.load_failed": "Impossibile caricare i valori di configurazione.",
        "setup.error.apply_failed": "Configurazione non riuscita.",

        "type.chat": "Chat",
        "type.repeater": "Ripetitore",
        "type.room": "Stanza",
        "type.sensor": "Sensore",
        "type.unknown": "Sconosciuto",

        "table.placeholder.no_nodes": "Nessun nodo trovato.",
        "table.column.type": "Tipo",
        "table.column.name": "Nome",
        "table.column.last": "Ultimo",

        "counter.channels_observed": "{count} canali ({observed} osservati)",
        "counter.channels": "{count} canali",
        "counter.nodes": "{count} nodi",

        "status.error": "Errore",

        "common.done": "Fatto",
        "common.cancel": "Annulla",
        "common.save": "Salva",

        "error.unknown": "Errore sconosciuto",
        "error.qr_library_missing": "La libreria del codice QR non è caricata.",
        "error.loading": "Errore durante il caricamento: {message}",
        "error.invalid_json": "risposta JSON non valida",
        "error.http": "Errore HTTP",
        "error.generic": "Errore",
        "room_password.input_placeholder": "Inserisci la password ..."
    }
};

let currentLanguage = localStorage.getItem("dashboardLanguage") || "de";

function getLanguage()
{
    return currentLanguage;
}

function setLanguage(lang)
{
    currentLanguage = I18N[lang] ? lang : "de";
    localStorage.setItem("dashboardLanguage", currentLanguage);
    applyTranslations();

    if (typeof renderChannelsList === "function")
    {
        renderChannelsList();
    }

    if (typeof renderChatMessages === "function")
    {
        renderChatMessages((typeof state !== "undefined" && state && Array.isArray(state.chatMessages)) ? state.chatMessages : []);
    }

    if (typeof renderDiscoverResults === "function")
    {
        renderDiscoverResults(
            (typeof state !== "undefined" && state) ? state.discoverResults : []
        );
    }

    if (typeof updateLeftCounter === "function")
    {
        updateLeftCounter();
    }

    if (typeof updatePageTitle === "function")
    {
        const nodeName =
            (typeof state !== "undefined" && state && state.deviceInfo && state.deviceInfo.node_name)
                ? state.deviceInfo.node_name
                : "";
        updatePageTitle(nodeName);
    }
}

function t(key, vars = {})
{
    const langMap = I18N[currentLanguage] || I18N.de;
    let text = langMap[key] || I18N.de[key] || key;

    Object.keys(vars).forEach(function(name)
    {
        text = text.replaceAll("{" + name + "}", String(vars[name]));
    });

    return text;
}

function applyTranslations()
{
    document.documentElement.lang = t("html.lang");
    document.title = t("document.title");

    document.querySelectorAll("[data-i18n]").forEach(function(node)
    {
        node.textContent = t(node.dataset.i18n);
    });

    document.querySelectorAll("[data-i18n-html]").forEach(function(node)
    {
        node.innerHTML = t(node.dataset.i18nHtml);
    });

    document.querySelectorAll("[data-i18n-placeholder]").forEach(function(node)
    {
        const translatedPlaceholder = t(node.dataset.i18nPlaceholder);

        if ("placeholder" in node)
        {
            node.placeholder = translatedPlaceholder;
        }

        node.setAttribute("data-placeholder", translatedPlaceholder);
    });

    document.querySelectorAll("[data-i18n-title]").forEach(function(node)
    {
        node.title = t(node.dataset.i18nTitle);
    });

    document.querySelectorAll("[data-i18n-value]").forEach(function(node)
    {
        node.value = t(node.dataset.i18nValue);
    });
}

document.addEventListener("DOMContentLoaded", function()
{
    const languageSelect = document.getElementById("languageSelect");

    if (languageSelect)
    {
        languageSelect.value = getLanguage();

        languageSelect.addEventListener("change", function()
        {
            setLanguage(languageSelect.value);
        });
    }

    applyTranslations();
});