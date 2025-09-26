return {
	"github/copilot.vim",
  event = "InsertEnter",
	config = function()
		vim.g.copilot_no_tab_map = true
		vim.g.copilot_assume_mapped = true
		vim.g.copilot_tab_fallback = ""

		vim.g.copilot_filetypes = {
			["*"] = false,
			["lua"] = true,
			["python"] = true,
			["sh"] = true,
			["zsh"] = true,
			["bash"] = true,
			["yaml"] = true,
			["json"] = true,
			["javascript"] = true,
			["typescript"] = true,
			["rust"] = true,
			["go"] = true,
			["c"] = true,
			["cpp"] = true,
			["java"] = true,
		}
		-- Accept Copilot
		vim.keymap.set("i", "<S-CR>", 'copilot#Accept("\\<CR>")', {
			expr = true,
			replace_keycodes = false,
			desc = "Accept Copilot",
		})
	end,
}
