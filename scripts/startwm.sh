#!/bin/sh

if [ -r /etc/default/locale ]; then
  . /etc/default/locale
  export LANG LANGUAGE
fi

if [ -f $HOME/.config/monitors.xml ]; then
  rm $HOME/.config/monitors.xml
fi

if [ -r /etc/X11/Xsession ]; then
  . /etc/X11/Xsession
elif [ -r /etc/gdm/Xsession ]; then
  . /etc/gdm/Xsession
elif [ -r /etc/X11/xinit/Xsession ]; then
  . /etc/X11/xinit/Xsession
elif [ -r /etc/X11/xinit/xinitrc ]; then
  . /etc/X11/xinit/xinitrc
fi

