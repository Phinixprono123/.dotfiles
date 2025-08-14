return {
	"neovim/nvim-lspconfig",
	config = function()
		-- List your servers here
		local servers = { "lua_ls", "pyright", "clangd", "ts_ls", "ruby_ls", "bashls", "asm_lsp" }

		-- Global defaults for all servers
		vim.lsp.config("*", {
			on_attach = function(client, bufnr)
				local map = function(mode, lhs, rhs)
					vim.keymap.set(mode, lhs, rhs, { buffer = bufnr, silent = true })
				end

				-- Common LSP keymaps
				map("n", "K", vim.lsp.buf.hover)
				map("n", "<leader>rn", vim.lsp.buf.rename)
				map("n", "gd", vim.lsp.buf.definition)
				map("n", "gD", vim.lsp.buf.declaration)
				map("n", "gr", vim.lsp.buf.references)
				map("n", "<leader>ca", vim.lsp.buf.code_action)

				-- Enable omnifunc for completion (your cmp will hook into this)
				vim.bo[bufnr].omnifunc = "v:lua.vim.lsp.omnifunc"
			end,
		})

		-- Load and enable each server
		for _, server in ipairs(servers) do
			local ok, conf = pcall(require, "lsp." .. server)
			if ok then
				vim.lsp.config(server, conf)
			end
			vim.lsp.enable(server)
		end
	end,
}
