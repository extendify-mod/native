# linux only for now
dir deps/wrapper
dir src
set stop-on-solib-events 0

#uncomment to break on load
# catch load libextendify.so

file result/share/spotify/.spotify-wrapped
# set-detach-on-fork off
set debuginfod enabled on
