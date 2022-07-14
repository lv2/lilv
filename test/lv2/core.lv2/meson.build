ttl_files = files(
  'lv2core.ttl',
  'manifest.ttl',
)

foreach file : ttl_files
  configure_file(copy: true, input: file, output: '@PLAINNAME@')
endforeach
