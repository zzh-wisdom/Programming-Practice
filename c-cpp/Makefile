BAZEL = bazel

# build -c dbg  # 调试模式
# build --config=disk
# build --copt=-ggdb3  # 调试信息
# build --copt=-O2
# --ignore_unsupported_sandboxing --verbose_failures
BUILD_OPT = -s -c opt --copt=-O2 --jobs 10  # opt 表示发布模式
# BUILD_OPT = -s -c dbg --copt=-ggdb3 --jobs 10  # dbg 表示调试模式

ALL_TARGET =

TEST_TOOLS = test_test test_test_smp util_test_spsc_test

.PHONY: all clean test_tools ${ALL_TARGET} ${TEST_TOOLS}

all: ${ALL_TARGET} test_tools

test_tools: ${TEST_TOOLS}

clean:
	${BAZEL} clean

test_test:
	${BAZEL} build ${BUILD_OPT} test:test

test_test_smp:
	${BAZEL} build ${BUILD_OPT} test:test_smp

util_test_spsc_test:
	${BAZEL} build ${BUILD_OPT} util/test:spsc_test
