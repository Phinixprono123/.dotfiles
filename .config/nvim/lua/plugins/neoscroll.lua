return {
	"karb94/neoscroll.nvim",
	opts = {
		hide_cursor = true,
		stop_on_keypress = true,
		easing_function = "quadratic", -- still respected
		performance_mode = true,
		direction_mode = "auto",
		framerate = 60,
		scroll_down_time = 350,
		scroll_up_time = 350,
	},
	config = function(_, opts)
		local neoscroll = require("neoscroll")
		neoscroll.setup(opts)

		local scroll = neoscroll.scroll

		-- PgUp: negative lines = up
		vim.keymap.set({ "n", "v", "x" }, "<PageUp>", function()
			scroll(-vim.wo.scroll, {
				move_cursor = true,
				duration = opts.scroll_up_time, -- NOTE: must be `duration` now
				easing = opts.easing_function,
			})
		end, { desc = "Smooth scroll up" })

		-- PgDn: positive lines = down
		vim.keymap.set({ "n", "v", "x" }, "<PageDown>", function()
			scroll(vim.wo.scroll, {
				move_cursor = true,
				duration = opts.scroll_down_time, -- again `duration`
				easing = opts.easing_function,
			})
		end, { desc = "Smooth scroll down" })
	end,
}
