Oracle for luce-browser-engine regions r24/r25 (the form controls).
./build.sh                       links oracle.cpp with the reference build's LibWeb objects
./oracle cases.txt > expected.txt
python3 gen_luce_cases.py > ../../../../luce-browser-engine/src/web/html/tests_form_controls_cases.lucb
Modes are described at the top of oracle.cpp; html/tests_form_controls.lucb is the port's side.
