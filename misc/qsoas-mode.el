;;; qsoas-mode.el --- major mode for QSoas command files

;; Copyright (C) 2012, 2016 Vincent Fourmond

;; This file is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.
;;
;; debian-mr-copyright-mode.el is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with your Debian installation, in /usr/share/common-licenses/GPL
;; If not, write to the Free Software Foundation, 675 Mass Ave,
;; Cambridge, MA 02139, USA.

(defvar qsoas-syntax-table nil
  "Syntax table used in qsoas-mode buffers.")

(if qsoas-syntax-table
    ()
  (setq qsoas-syntax-table (make-syntax-table))
  ;; Support # style comments
  (modify-syntax-entry ?#  "<"  qsoas-syntax-table)
  (modify-syntax-entry ?\n "> " qsoas-syntax-table)
  (modify-syntax-entry ?\{ "(}" qsoas-syntax-table)
  (modify-syntax-entry ?\} "){" qsoas-syntax-table)
  (modify-syntax-entry ?\' "\"'" qsoas-syntax-table)
  )

(defvar qsoas-available-commands nil
  "List of available commands")

(defun qsoas-command-list ()
  "Returns the list of commands known to QSoas. Results are cached"
  (if qsoas-available-commands
      ()
    (setq qsoas-available-commands 
          (process-lines "QSoas" "--list-commands"))
    )
  qsoas-available-commands
  )

(defun qsoas-make-font-lock ()
  "This returns a neat font-lock table"
  (list
   ;; Command names
   (list
    (concat
     "^[[:blank:]]*\\<\\("
     (regexp-opt (qsoas-command-list))
     "\\)\\([[:blank:]]+\\|$\\|[[:blank:]]*(\\)")
    1 font-lock-function-name-face
    )
   '("\\${\\([[:alnum:]_:+-%#]+\\)}"
     1
     font-lock-variable-name-face t)
   )
  )

(defvar qsoas-regexp-alist
  '(("file '\\([^']+\\)' +line +\\([0-9]+\\)" 1 2))
  "Regexp used to match qsoas errors.  See `compilation-error-regexp-alist'.")



;;;###autoload
(define-derived-mode qsoas-mode fundamental-mode "qsoas"
  "A major mode for editing qsoas command files"
  (set-syntax-table qsoas-syntax-table)
  ;; Comments
  (make-local-variable 'comment-start-skip)  ;Need this for font-lock...
  (setq comment-start-skip "\\(^\\|\\s-\\);?#+ *") ;;From perl-mode
  (make-local-variable 'comment-start)
  (make-local-variable 'comment-end)
  (setq comment-start "# " comment-end "")

  (make-local-variable 'font-lock-defaults)
  (setq font-lock-defaults 
        (list (qsoas-make-font-lock)
              nil           ;;; Keywords only? No, let it do syntax via table.
              nil           ;;; case-fold?
              nil           ;;; Local syntax table.
              nil           ;;; Use `backward-paragraph' ? No
              )
        )
  )

