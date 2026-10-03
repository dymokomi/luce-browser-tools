# Writes cases.txt: the generated property data of every property (and of custom properties), the
# value type of every environment variable, every property's initial value, every descriptor's
# initial value, then the hand-written cases of value_cases.txt. Lines of later.txt (one case per
# line, as in cases.txt) are the cases whose path reaches other regions' stubs in the port; they
# are written with the "!" prefix.
import json
D = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Libraries/LibWeb/CSS/'
properties = json.load(open(D + 'Properties.json'))
env = json.load(open(D + 'EnvironmentVariables.json'))
descriptors = json.load(open(D + 'Descriptors.json'))
later = set(l.rstrip('\n').rstrip('\t') for l in open('later.txt', encoding='utf-8') if l.strip() and not l.startswith('#'))
lines = ['# The generated data of every property (PropertyID.cpp): accepted types, ranges, percentage basis, range checks.']
lines += ['property\tcustom'] + ['property\t' + name for name, value in properties.items() if 'legacy-alias-for' not in value]
lines += ['# The value type of every environment variable (EnvironmentVariable.cpp).']
lines += ['envvar\t' + name for name in env]
lines += ['# Every initial value (PropertyID.cpp property_initial_value).']
# The donor's own initial values of these do not parse: property_initial_value fails its VERIFY (and the
# port traps the same way).
no_initial = {'grid', 'grid-template'}
# Likewise for these descriptors (the generated C++ literal of pad's '0 ""' is "0 " "", i.e. "0 ").
no_descriptor_initial = {('counter-style', 'negative'), ('counter-style', 'prefix'), ('counter-style', 'suffix'), ('counter-style', 'pad')}
lines += ['initial\t' + name for name, value in properties.items() if 'legacy-alias-for' not in value and name not in no_initial]
lines += ['# Every descriptor initial value (DescriptorID.cpp descriptor_initial_value).']
for at_rule, data in descriptors.items():
    for name, value in data.get('descriptors', {}).items():
        if 'legacy-alias-for' not in value and (at_rule, name) not in no_descriptor_initial:
            lines.append('descriptor_initial\t@' + at_rule + '\t' + name)
lines += [l.rstrip('\n') for l in open('value_cases.txt', encoding='utf-8')]
with open('cases.txt', 'w', encoding='utf-8') as out:
    for line in lines:
        out.write(('!' + line if line.rstrip('\t') in later else line) + '\n')
