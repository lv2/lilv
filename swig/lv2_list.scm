; Least idiomatic scheme bindings ever.  Work in progress...

(require-extension slv2)

(define world (slv2-world-new))
(slv2-world-load-all world)

(define plugins (slv2-world-get-all-plugins world))

(let ((p (slv2-plugins-get-at plugins 0)))
  (display (slv2-value-as-string (slv2-plugin-get-uri p)))
  (newline))

