#!/usr/bin/osascript
-- https://medium.com/@sunskyearthwind/iterm-applescript-and-jumping-quickly-into-your-workflow-1849beabb5f7
-- https://www.iterm2.com/documentation-scripting.html
-- https://stackoverflow.com/questions/48435951/how-to-write-text-to-a-iterm2-session-in-applescript

tell application "Finder"
 set displayAreaDimensions to bounds of window of desktop
 set x1 to item 1 of displayAreaDimensions
 set y1 to item 2 of displayAreaDimensions
 set x2 to item 3 of displayAreaDimensions
 set y2 to item 4 of displayAreaDimensions

# get size of main screen
# https://stackoverflow.com/questions/1866912/applescript-how-to-get-current-display-resolution
 set dimensions to words of (do shell script "system_profiler SPDisplaysDataType | awk '/Main Display: Yes/{found=1} /Resolution/{w2=$2; h2=$4} /UI Looks like/{w1=$4; h1=$6} /Retina/{scale=($2 == \"Yes\" ? 2 : 1)} /^ {8}[^ ]+/{if(found) {exit}; scale=1} END{printf \"%d %d %d\\n\", w1?w1:w2, h1?h1:h2, scale}'")
end tell

set width to item 1 of dimensions
set height to item 2 of dimensions

tell application "iTerm2"
  tell current tab of current window
    #set mysession to first item of sessions
    set _mysession to current session
  end tell

  tell first session of current tab of current window
    write text "# make flash port=3"
  end tell

  tell current window
    create tab with default profile
  end tell

  tell first session of current tab of current window
    # set name to "backend"
    # write text "cd backend; clear"
  end tell

  # create new window
  create window with default profile
  set the bounds of the first window to {x2 - 2*width/3, y2 - height/3, x2, y2}
  create tab with default profile


  tell first session of current tab of current window
    set name to "attiny3217"
    set background color to {255*65535/255, 255*65535/255, 200*65535/255}
    split vertically with default profile
    write text "make serial port=1"
  end tell

  tell second session of current tab of current window
    set name to "esp32"
    set background color to {200*65535/255, 255*65535/255, 200*65535/255}
    write text "cd /www/esp32-devboard; make terminalt port=4"
  end tell

#  tell _mysession
#    activate
#    select
#    #set frontmost of window id _my to true
#    write text "echo 'foo'"
#  end tell
end tell

# set focus to 1st screen
tell application "System Events" to tell process "iTerm2"
  set frontmost to true
  windows where title contains "backend"
  if result is not {} then perform action "AXRaise" of item 1 of result
end tell
