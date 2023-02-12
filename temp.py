
import os

for path, dirs, files in os.walk('./edgelists'):
    for name in files:
        a = os.path.join(path, name)
        b = os.path.join('./edgelists2', name)
        f1 = open(a, 'r')
        f2 = open(b, 'w')
        print(a, b)
        for line in f1.readlines():
            t = line.strip().split(' ')
            if len(t) == 2:
                x = int(t[0])
                y = int(t[1])
                if x < y:
                    f2.write(str(x) + ' ' + str(y) + '\n')
        f1.close()
        f2.close()
