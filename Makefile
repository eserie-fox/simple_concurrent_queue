BIN_PATH:=./bin

COMPILER:=c++

run_test : test example1 example2 example_cpp11
	$(BIN_PATH)/test
	$(BIN_PATH)/example1
	$(BIN_PATH)/example2
	$(BIN_PATH)/example_cpp11

test : $(BIN_PATH)/test

example1 : $(BIN_PATH)/example1

example2 : $(BIN_PATH)/example2

example_cpp11 : $(BIN_PATH)/example_cpp11

$(BIN_PATH)/test : test/test.cc concurrent_queue.h third_party/catch.hpp | build_prepare
	$(COMPILER) $< -o $@

$(BIN_PATH)/example1 : example/example1.cc concurrent_queue.h | build_prepare
	$(COMPILER) $< -o $@

$(BIN_PATH)/example2 : example/example2.cc concurrent_queue.h | build_prepare
	$(COMPILER) $< -o $@

$(BIN_PATH)/example_cpp11 : example/example_cpp11.cc concurrent_queue.h | build_prepare
	$(COMPILER) $< -o $@ -std=c++11

build_prepare:
	@mkdir -p $(BIN_PATH)

clean: 
	@rm -f $(BIN_PATH)/test $(BIN_PATH)/example1 $(BIN_PATH)/example2 $(BIN_PATH)/example_cpp11
	@if [ -d "$(BIN_PATH)" ] && [ -z "$$(ls -A $(BIN_PATH))" ]; then \
		rmdir $(BIN_PATH); \
	fi