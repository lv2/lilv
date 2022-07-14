c_lilv_rst = custom_target(
  'lilv.rst',
  command: [dox_to_sphinx, '-f', '@INPUT0@', '@OUTDIR@'],
  input: [c_index_xml] + c_rst_files,
  output: 'lilv.rst',
)
