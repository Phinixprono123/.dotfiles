return {
	"vyfor/cord.nvim",
	build = ":Cord update",
	-- opts = {}
	config = function()
		require("cord").setup({
			editor = {
				tooltip = "What are you doing here?",
			},
		})
	end,
}
