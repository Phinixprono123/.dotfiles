return {
  {
    "williamboman/mason.nvim",
    opts = {
      automatic_installation = true,
      ensure_installed = {
        "ruff",
        "prettierd",
        "isort",
        "stylua",
        "lua-language-server",
        "shellcheck",
        "shfmt",
        "flake8",
        "css-lsp",
        "html-lsp",
        "typescript-language-server",
        "eslint_d",
        "clangd",
        "clang-format",
      },
    },
  },
}
