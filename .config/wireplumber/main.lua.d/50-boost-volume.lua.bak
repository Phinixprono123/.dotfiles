table.insert(config.monitors, {
	name = "volume-boost",
	rules = {
		{
			matches = {
				{
					{ "node.name", "matches", ".*" },
				},
			},
			apply_properties = {
				["volume.max"] = 1.5, -- 150%
				["volume.soft-limit"] = 1.5,
			},
		},
	},
})
