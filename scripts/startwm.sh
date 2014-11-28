#!/bin/sh

if [ -r /etc/default/locale ]; then
  . /etc/default/locale
  export LANG LANGUAGE
fi

# /usr/share/xsessions contains available X11 session types

if [ -r /usr/lib/gnome-panel/gnome-session-flashback ]; then
  exec /usr/lib/gnome-panel/gnome-session-flashback
elif [ -r /bin/mate-session ]; then
  exec /bin/mate-session
elif [ -r /etc/X11/Xsession ]; then
  . /etc/X11/Xsession
elif [ -r /etc/X11/xinit/Xsession ]; then
  . /etc/X11/xinit/Xsession
elif [ -r /etc/gdm/Xsession ]; then
  . /etc/gdm/Xsession
elif [ -r /etc/X11/xinit/xinitrc ]; then
  . /etc/X11/xinit/xinitrc
fi

