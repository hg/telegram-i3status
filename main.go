package main

import (
	"fmt"
	"log"

	"github.com/godbus/dbus/v5"
)

func importance(count int64) string {
	switch true {
	case count == 0:
		return "Idle"

	case count < 10:
		return "Info"

	case count < 50:
		return "Warning"

	default:
		return "Critical"
	}
}

func makeSetCount(conn *dbus.Conn) func(count int64) {
	panel := conn.Object("i3.status.rs", "/telegram")

	return func(count int64) {
		color := importance(count)
		text := fmt.Sprintf("%d", count)

		call := panel.Call("i3.status.rs.SetStatus", 0, text, "bell", color)

		if call.Err != nil {
			log.Println("failed to call SetStatus", call.Err)
		}
	}
}

func main() {
	bus, err := dbus.ConnectSessionBus()
	if err != nil {
		log.Fatalln("failed to connect to session bus", err)
	}
	defer bus.Close()

	err = bus.AddMatchSignal(
		dbus.WithMatchInterface("com.canonical.Unity.LauncherEntry"),
		dbus.WithMatchMember("Update"),
		dbus.WithMatchArg(0, "application://org.telegram.desktop.desktop"),
	)

	if err != nil {
		log.Fatalln("failed to eavesdrop on dbus", err)
	}

	signals := make(chan *dbus.Signal, 4)
	bus.Signal(signals)

	setCount := makeSetCount(bus)

	for sig := range signals {
		if len(sig.Body) != 2 {
			log.Println("unexpected signal body", sig.Body)
			continue
		}

		args, ok := sig.Body[1].(map[string]dbus.Variant)

		if !ok {
			log.Println("unexpected 2nd argument", sig.Body[1])
			continue
		}

		visible := args["count-visible"].Value().(bool)

		if visible {
			count := args["count"].Value().(int64)
			setCount(count)
		} else {
			setCount(0)
		}
	}
}
