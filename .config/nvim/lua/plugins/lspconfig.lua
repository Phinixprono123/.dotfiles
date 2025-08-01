return {
  {
    "neovim/nvim-lspconfig",
    opts = {
      servers = {
        ruff = {},
        cssls = {},
        html = {},
        lua_ls = {},
        ts_ls = {},
        eslint = {},
        clangd = {},
      },
    },
  },
}
