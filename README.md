Connects Telegram unread counter to [`i3status-rs`][i3rs].

[i3rs]: https://github.com/greshake/i3status-rust

Add this to `config.toml`:

```toml
[[block]]
block = "custom_dbus"
name = "telegram"
initial_text = ""
```

Build the program and put it into `~/.local/bin`.

Create a systemd user unit to start it and keep it running:

    $ systemctl --user edit --full --force telegram-i3status.service

```ini
[Unit]
BindsTo = sway-session.target

[Install]
WantedBy = sway-session.target

[Service]
Type = simple
ExecStart = %h/.local/bin/telegram-i3status
Slice = background.slice
Restart = always
RestartSec = 5
```

TODO: socket activation?
