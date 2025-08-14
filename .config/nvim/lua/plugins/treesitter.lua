return {
  "nvim-treesitter/nvim-treesitter",
  branch = "master",
  lazy = false,
  build = ":TSUpdate",
  config = function()
    require("nvim-treesitter.configs").setup {
      ensure_installed = {
        "c", "lua", "vim", "vimdoc", "query",
        "markdown", "markdown_inline", "cpp",
        "json", "jsonc", "python", "rust", "ruby",
        "yuck", "css", "scss", "javascript", "typescript",
	"zig", "asm", "yaml", "toml", "bash", "bass", "c_sharp",
	"cooklang", "csv", "d", "dart", "desktop", "diff", "erlang",
	"fish", "gdscript", "git_config", "gitcommit", "go", "hyprlang",
	"http", "java", "kotlin", "make", "nix", "php", "powershell", "properties",
	"sql", "tmux",
      },
      highlight = { enable = true },
      indent = { enable = true },
    }
  end,
}

