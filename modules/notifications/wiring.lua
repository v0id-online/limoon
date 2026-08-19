-- wiring.lua — Connects existing Li Moon events to the notification catalog.
--
-- messages.lua and bar.lua are foundation pieces that don't wire
-- themselves to anything; this file is the other side — it listens to
-- events core/file_io.lua and modules/limoon/find.lua already emit and
-- turns them into notify.* calls, without modifying those files (the one
-- exception, search_no_matches, is documented in find.lua itself, since
-- that specific case has no dedicated event to listen for).

local notify = require('notifications.bar')

-- File
events.connect(events.FILE_OPENED, function(filename)
	notify.info('file_opened', filename or 'Untitled')
end)
events.connect(events.FILE_AFTER_SAVE, function(filename)
	notify.success('file_saved', filename or 'Untitled')
end)
-- `buffer` is still the buffer being closed when this fires (see
-- events.lua's own doc comment on BUFFER_DELETED).
events.connect(events.BUFFER_DELETED, function()
	notify.info('file_closed', buffer.filename or 'Untitled')
end)

-- Search
events.connect(events.FIND_WRAPPED, function() notify.info('search_wrapped') end)
