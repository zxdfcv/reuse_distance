import random

list = []
with open("E:\\CUDA\\splay\\test4.txt", "w") as fp:
    for i in range(1, 1001):
        #x = random.randint(1, 1000)
        if i == 130:
            list.append(100)
            fp.write("100\n")
        else:
            list.append(i)
            fp.write(str(i) + "\n")
print(list)
