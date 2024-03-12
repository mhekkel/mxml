project = 'mxml-doc'
copyright = '2024, Maarten L. Hekkelman'
author = 'Maarten L. Hekkelman'
release = '1.0.1'

# -- General configuration ---------------------------------------------------

extensions = [
    "breathe",
    "exhale",
    "myst_parser"
]

breathe_projects = {
	"mxml": "../build/docs/xml"
}

myst_enable_extensions = [ "colon_fence" ]
breathe_default_project = "mxml"

# Setup the exhale extension
exhale_args = {
    # These arguments are required
    "containmentFolder":     "./api",
    "rootFileName":          "library_root.rst",
    "doxygenStripFromPath":  "../include/",
    # Heavily encouraged optional argument (see docs)
    "rootFileTitle":         "API Reference",
    # Suggested optional arguments
    # "createTreeView":        True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
    "exhaleExecutesDoxygen": False,
    "contentsDirectives" : False,
    
    "verboseBuild": False
}

# Tell sphinx what the primary language being documented is.
primary_domain = 'cpp'

# Tell sphinx what the pygments highlight language should be.
highlight_language = 'cpp'

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_theme_options = {
}

cpp_index_common_prefix = [
	'mxml::',
]


