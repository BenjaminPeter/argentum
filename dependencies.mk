InputReader.o: InputReader.cpp InputReader.h default.h
Tree.o: Tree.cpp Tree.h default.h
TreeController.o: TreeController.cpp TreeController.h default.h Tree.h
TreeControllerSimple.o: TreeControllerSimple.cpp TreeControllerSimple.h \
  default.h Tree.h
main-simple.o: main-simple.cpp InputReader.h default.h Tree.h \
  TreeControllerSimple.h
main.o: main.cpp InputReader.h default.h Tree.h TreeController.h
rand-matrix.o: rand-matrix.cpp
