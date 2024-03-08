---
layout: post
title: SDDM + KDE Plasma Wayland Frozen After Login
tags: kde wayland
---

I could not log into my computer this morning.  I was using [SDDM] + [KDE][KDE]
[Plasma] with [Wayland].  When I typed my password and pressed ENTER, it stopped
responding.  The screen remained on the log-in screen, but the mouse cursor
disappeared.  I tried to switch terminal using CTRL+ALT+Fx (x = 2, 3, ..., 8),
but that didn't work, either.  What was wrong with it?  I kept trying and
eventually got the answer.

[SDDM]: https://github.com/sddm/sddm
[KDE]: https://kde.org/
[Plasma]: https://kde.org/plasma-desktop/
[Wayland]: https://community.kde.org/KWin/Wayland

# Rescue

Since CTRL+ALT+Fx didn't work, I tried to log into my machine from another
machine via SSH.  It worked.  I managed to get a shell, and restarted SDDM using
`sudo systemctl restart sddm`.  Then my computer became responsive again.

# What was wrong?

Many things could go wrong.  I updated quite some packages yesterday, and it was
ArchLinux.  As a rolling Linux distribution, any update could render the system
unusable.  But when I visited <https://archlinux.org/>, no recent news mentioned
anything that needed manual intervention.

I was using NVidia's open source driver (the [`nvidia-open`] package).  Could
that be a problem?  I uninstalled that package and rebooted.  It would still be
frozen after logging in.  So the display driver was not the problem.

[`nvidia-open`]: https://archlinux.org/packages/extra/x86_64/nvidia-open/

I was using KDE Plasma Wayland session.  Could that be a problem?  I tried to
start the session by typing `startplasma-wayland` on a TTY, and that started the
session successfully for me.  So Plasma Wayland may not be the problem.

I was using SDDM, the default display manager for KDE Plasma.  Could that be a
problem?  I installed LXDM.  LXDM could start the Plasma X11 session, but it was
incapable of starting any Wayland sessions.  I installed LightDM, but for some
reasons it failed to start.  I installed GDM.  It could start the GNOME session,
but it didn't detect the presence of Plasma Wayland.  In the end, I couldn't
find any display manager that could start the Plasma Wayland session besides
SDDM.  It could be SDDM's problem, but I couldn't confirm it.

I tried to created another user, and tried to log in and start the Plasma
Wayland session via SDDM.  Surprisingly, it worked.  It gave my new user a
desktop with default settings.  This meant the problem was probably with my
personal configuration.

What configuration?

I was using [Fish](https://fishshell.com) as my default shell.  I used `chsh` to
change my default shell to Bash, and tried to log in.  It worked.  It logged
into the Plasma Wayland session successfully.  So it narrowed down the problem
with my Fish configuration.

I moved my entire Fish configuration directory `~/.config/fish` away, and
attempted to login.  It worked.  That further confirmed that one (or more) of
my Fish configuration file was wrong.  Which one was it?

I tried to move my configuration files one by one from my backup location back
to `~/.config/fish`.  It ended up that the file `fish_variables` was the
culprit.

I then tried to gradually remove the lines from `fish_variables`.  In the end,
the culprit was the following line:

```
SETUVAR --export UID:1000
```

It is a universal variable.  It means it could have been set at any moment in
the past, and it persisted until today and caused the failure.

# Solution

Removing the universal variable `UID` solved the problem, but I still don't
understand why that variable was set in the first place.  And I don't know why
it was frozen when the `UID` environment variable was set.  Actually I don't
know exactly what was frozen, the SDDM, or something in the Plasma Wayland
session.

{% comment %}
vim: tw=80
{% endcomment %}
