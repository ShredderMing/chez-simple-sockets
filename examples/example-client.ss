#!/usr/bin/scheme --script

;; Copyright (C) 2016 Chris Vine
;; 
;; This file is licensed under the Apache License, Version 2.0 (the
;; "License"); you may not use this file except in compliance with the
;; License.  You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
;; implied.  See the License for the specific language governing
;; permissions and limitations under the License.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; This is an example file for using asynchronous reads and writes on
;; sockets.  It will provide the caller's IPv4 internet address from
;; myip.dnsdynamic.org.  Normally if you wanted to do this from a
;; utility script, you would do it synchronously with blocking
;; operations.  However in a program using an event loop, you would
;; need to do it asynchronously.  This does so.
;;
;; This file uses the chez-a-sync library from
;; https://github.com/ChrisVine/chez-a-sync

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(import (a-sync coroutines)
	(a-sync event-loop)
	(simple-sockets basic)
	(simple-sockets a-sync)
	(chezscheme))

(define check-ip "myip.dnsdynamic.org")

(define (await-read-response await resume sockport)
  (define header "")
  (define body "")
  (await-geteveryline! await resume sockport
		       (lambda (line)
			 (cond
			  [(not (string=? body ""))
			   (set! body (string-append body "\n" line))]
			  [(string=? line "")
			   (set! body (string (integer->char 0)))] ;; marker
			  [else
			   (set! header (if (string=? header "")
					    line
					    (string-append header "\n" line)))])))
  ;; get rid of marker (with \n) in body
  (set! body (substring body 2 (string-length body)))
  (values header body))

(define (await-send-get-request await resume host path sockport)
  (await-put-string! await resume sockport
		     (string-append "GET " path " HTTP/1.1\nHost: "host"\n\n")))

(define (make-sockport codec socket)
  (let ([sockport (open-fd-input/output-port socket
					     (buffer-mode block)
					     (make-transcoder codec 'crlf))])
    ;; make the output port unbuffered
    (set-textual-port-output-size! sockport 0)
    ;; and make the socket non-blocking
    (set-port-nonblocking! sockport #t)
    sockport))

(set-default-event-loop!)

(a-sync
 (lambda (await resume)
   ;; getaddrinfo in particular can block, so call it up with
   ;; either await-task-in-thread! or await-task-in-event-loop!
   (let* ([socket (await-task-in-thread! await resume
					(lambda ()
					  (connect-to-ipv4-host check-ip "http" 0)))]
	  [sockport (make-sockport (utf-8-codec) socket)])
     (await-send-get-request await resume check-ip "/" sockport)
     (let-values ([(header body) (await-read-response await resume sockport)])
       (display body)
       (newline))
     (event-loop-block! #f)
     (clear-input-port)
     (close-port sockport))))

(event-loop-block! #t)
(event-loop-run!)