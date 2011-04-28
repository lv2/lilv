; Least idiomatic scheme bindings ever.  Work in progress...

(require-extension lilv)

(define world (lilv-world-new))
(lilv-world-load-all world)

(define plugins (lilv-world-get-all-plugins world))

(let ((p (lilv-plugins-get-at plugins 0)))
  (display (lilv-value-as-string (lilv-plugin-get-uri p)))
  (newline))

