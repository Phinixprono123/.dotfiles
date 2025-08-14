-- ~/.config/gBar/init.lua

local config = {
	-- bar placement
	position = "top",
	monitor = 0,
	height = 30,
	style = "style.scss",
	update_interval = 1,
}

config.modules = {
	-- 1. Workspaces (Hyprland)
	workspaces = {},

	-- 2. CPU usage
	cpu = {
		update_interval = 15,
		format = " {usage}%",
		max_length = 100,
	},

	-- 3. Memory usage
	memory = {
		update_interval = 30,
		format = " {usage}%",
		max_length = 10,
	},

	-- 4. System tray
	tray = {
		icon_size = 18,
		spacing = 10,
	},

	-- 5. Active window title (custom/window)
	custom_window = {
		command = "~/.config/waybar/custom_modules/windows_title.sh",
		update_interval = 1,
	},

	-- 6. PulseAudio volume
	pulseaudio = {
		tooltip = false,
		format = "{icon} {volume}%",
		format_muted = " {volume}%",
		on_click = "pactl set-sink-mute @DEFAULT_SINK@ toggle",
		on_scroll_up = "~/.config/waybar/custom_modules/volume.sh",
		on_scroll_down = "pactl set-sink-volume @DEFAULT_SINK@ -5%",
		format_icons = { "", "", "" },
	},

	-- 7. Clock
	clock = {
		tooltip = true,
		format = "%I:%M:%p",
		tooltip_format = "%Y-%m-%d",
	},

	-- 8. Power menu (custom/power)
	custom_power = {
		command = "nwg-bar",
		format = "",
		tooltip = false,
	},

	-- 9. (Optional) Network status
	network = {
		tooltip = false,
		format_wifi = "  {essid}",
		format_ethernet = "",
	},
}

return config
