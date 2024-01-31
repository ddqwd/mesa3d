

# framework基本内容

```
/framework/
├── backends
│   ├── abstract.py
│   ├── compression.py
│   ├── __init__.py
│   ├── json.py
│   ├── junit.py
│   ├── __pycache__
│   │   ├── abstract.cpython-310.pyc
│   │   ├── compression.cpython-310.pyc
│   │   ├── __init__.cpython-310.pyc
│   │   ├── json.cpython-310.pyc
│   │   ├── junit.cpython-310.pyc
│   │   └── register.cpython-310.pyc
│   └── register.py
├── core.py
├── dmesg.py
├── exceptions.py
├── grouptools.py
├── __init__.py
├── log.py
├── monitoring.py
├── options.py
├── os
├── profile.py
├── programs
│   ├── __init__.py
│   ├── parsers.py
│   ├── print_commands.py
│   ├── __pycache__
│   │   ├── __init__.cpython-310.pyc
│   │   ├── parsers.cpython-310.pyc
│   │   ├── print_commands.cpython-310.pyc
│   │   ├── run.cpython-310.pyc
│   │   └── summary.cpython-310.pyc
│   ├── run.py
│   └── summary.py
├── __pycache__
│   ├── core.cpython-310.pyc
│   ├── core.cpython-37.pyc
│   ├── dmesg.cpython-310.pyc
│   ├── dmesg.cpython-37.pyc
│   ├── exceptions.cpython-310.pyc
│   ├── exceptions.cpython-37.pyc
│   ├── grouptools.cpython-310.pyc
│   ├── grouptools.cpython-37.pyc
│   ├── __init__.cpython-310.pyc
│   ├── __init__.cpython-37.pyc
│   ├── log.cpython-310.pyc
│   ├── log.cpython-37.pyc
│   ├── monitoring.cpython-310.pyc
│   ├── monitoring.cpython-37.pyc
│   ├── options.cpython-310.pyc
│   ├── options.cpython-37.pyc
│   ├── profile.cpython-310.pyc
│   ├── profile.cpython-37.pyc
│   ├── results.cpython-310.pyc
│   ├── results.cpython-37.pyc
│   ├── status.cpython-310.pyc
│   ├── status.cpython-37.pyc
│   ├── wflinfo.cpython-310.pyc
│   └── wflinfo.cpython-37.pyc
├── replay
│   ├── backends
│   │   ├── abstract.py
│   │   ├── apitrace.py
│   │   ├── gfxreconstruct.py
│   │   ├── __init__.py
│   │   ├── register.py
│   │   ├── renderdoc
│   │   │   └── renderdoc_dump_images.py
│   │   └── renderdoc.py
│   ├── compare_replay.py
│   ├── download_utils.py
│   ├── image_checksum.py
│   ├── __init__.py
│   ├── options.py
│   ├── programs
│   │   ├── checksum.py
│   │   ├── compare.py
│   │   ├── download.py
│   │   ├── dump.py
│   │   ├── __init__.py
│   │   ├── parsers.py
│   │   └── query.py
│   └── query_traces_yaml.py
├── results.py
├── status.py
├── summary
│   ├── common.py
│   ├── console_.py
│   ├── feature.py
│   ├── html_.py
│   ├── __init__.py
│   └── __pycache__
│       ├── common.cpython-310.pyc
│       ├── console_.cpython-310.pyc
│       ├── feature.cpython-310.pyc
│       ├── html_.cpython-310.pyc
│       └── __init__.cpython-310.pyc
├── test
│   ├── base.py
│   ├── deqp.py
│   ├── glsl_parser_test.py
│   ├── gtest.py
│   ├── __init__.py
│   ├── oclconform.py
│   ├── opencv.py
│   ├── opengl.py
│   ├── piglit_test.py
│   ├── __pycache__
│   │   ├── base.cpython-310.pyc
│   │   ├── base.cpython-37.pyc
│   │   ├── glsl_parser_test.cpython-310.pyc
│   │   ├── glsl_parser_test.cpython-37.pyc
│   │   ├── gtest.cpython-310.pyc
│   │   ├── gtest.cpython-37.pyc
│   │   ├── __init__.cpython-310.pyc
│   │   ├── __init__.cpython-37.pyc
│   │   ├── oclconform.cpython-310.pyc
│   │   ├── oclconform.cpython-37.pyc
│   │   ├── opencv.cpython-310.pyc
│   │   ├── opencv.cpython-37.pyc
│   │   ├── opengl.cpython-310.pyc
│   │   ├── opengl.cpython-37.pyc
│   │   ├── piglit_test.cpython-310.pyc
│   │   ├── piglit_test.cpython-37.pyc
│   │   ├── shader_test.cpython-310.pyc
│   │   ├── shader_test.cpython-37.pyc
│   │   ├── xorg.cpython-310.pyc
│   │   └── xorg.cpython-37.pyc
│   ├── shader_test.py
│   └── xorg.py
└── wflinfo.py

```

# framework.log


## LogManager

```py
__all__ = ['LogManager']

class LogManager(object):
    """创建新的日志对象

    它创建的每个日志对象都具有两个可访问的方法：start() 和 log()；
    两个方法都不应返回任何内容，并且它们必须是线程安全的。
    log() 应该接受一个参数，即测试返回的状态。
    start() 也应该接受一个参数，即测试的名称。

    初始化 LogManager 时，它使用一个参数进行初始化，该参数确定它将返回哪个日志类
    （它们通过 LOG_MAP 字典设置，该字典将字符串名称映射到类的可调用方法），
    只要 LogManager 存在，它将返回这些类。

    参数：
    logger -- 要使用的日志记录器的字符串名称
    total -- 要运行的测试的总数

    """
    LOG_MAP = {
        'quiet': QuietLog,
        'verbose': VerboseLog,
        'dummy': DummyLog,
        'http': HTTPLog,
    }

    def __init__(self, logger, total):
        assert logger in self.LOG_MAP
        self._log = self.LOG_MAP[logger]
        self._state = {
            'total': total,
            'summary': collections.defaultdict(lambda: 0),
            'lastlength': 0,
            'complete': 0,
            'running': [],
        }
        self._state_lock = threading.Lock()

        # 为 HTTP 日志记录器启动 HTTP 服务器
        if logger == 'http':
            self.log_server = HTTPLogServer(self._state, self._state_lock)
            self.log_server.start()

    def get(self):
        """返回一个新的日志实例"""
        return self._log(self._state, self._state_lock)

```

log类型分为    LOG_MAP = {
        'quiet': QuietLog,
        'verbose': VerboseLog,
        'dummy': DummyLog,
        'http': HTTPLog,
    }


## QuietLog

```
class QuietLog(BaseLog):
    """提供最小状态输出的日志记录器

    该日志记录器仅提供类似 [x/y] pass: z, fail: w 的输出。

    在打印到 tty 时，它使用 \r 来覆盖相同行。如果 sys.stdout 不是 tty，则在行末打印 \n。

    参数：
    status -- 要打印的状态

    """
    _test_counter = itertools.count()

    def __init__(self, *args, **kwargs):
        super(QuietLog, self).__init__(*args, **kwargs)

        # 如果输出是 tty，则可以使用 '\r'（回车，无换行）来覆盖现有行，
        # 如果 stdout 不是 tty，则这将无效（并可能破坏像 Jenkins 这样的系统），因此我们会打印 \n。
        if sys.stdout.isatty():
            self._endcode = '\r'
        else:
            self._endcode = '\n'

        self.__counter = next(self._test_counter)

    def start(self, name):
        # 这不能在构造函数中完成，因为构造函数也会用于最终摘要。
        with self._LOCK:
            self._state['running'].append(self.__counter)

    def _log(self, status):
        """用于记录的非锁定辅助函数

        为了允许与使用非递归锁的子类共享代码，我们需要以未锁定的形式共享此代码。

        """
        # 增加 complete
        self._state['complete'] += 1

        # 添加到摘要字典
        assert status in self.SUMMARY_KEYS, \
            'Logger 的无效状态: {}'.format(status)
        self._state['summary'][status] += 1

        self._print_summary()
        self._state['running'].remove(self.__counter)

    def log(self, status):
        with self._LOCK:
            self._log(status)

    def summary(self):
        with self._LOCK:
            self._print_summary()
            self._print('\n')

    def _print_summary(self):
        """打印摘要结果

        这会打印 '[done/total] {status}'，它是一个对于该类的子类（VerboseLog）隐藏的私有方法。

        """
        assert self._LOCK.locked()

        out = '[{done}/{total}] {status} {running}'.format(
            done=str(self._state['complete']).zfill(self._pad),
            total=str(self._state['total']).zfill(self._pad),
            status=', '.join('{0}: {1}'.format(k, v) for k, v in
                             sorted(self._state['summary'].items())),
            running=''.join('|/-\\'[x % 4] for x in self._state['running'])
        )

        self._print(out)

    def _print(self, out):
        """确保覆盖任何错误行的共享打印方法"""
        assert self._LOCK.locked()

        # 如果新行比旧行短，则添加尾随空格
        pad = self._state['lastlength'] - len(out)

        sys.stdout.write(out)
        if pad > 0:
            sys.stdout.write(' ' * pad)
        sys.stdout.write(self._endcode)
        sys.stdout.flush()

        # 设置刚打印行的长度（减去空格，因为我们不关心留下空格），
        # 以便下一行将在必要时打印额外的空格
        self._state['lastlength'] = len(out)

```

## VerboseLog

```python
class VerboseLog(QuietLog):
    """QuietLog 的更详细版本

    该日志记录器的工作方式类似于 QuietLog，只是它永远不会覆盖上一行。
    它还在每个测试开始时打印一行额外的行，这是 quite 日志记录器不执行的操作。

    """
    def __init__(self, *args, **kwargs):
        super(VerboseLog, self).__init__(*args, **kwargs)
        self.__name = None    # 名称需要由 start 和 log 打印

    def _print(self, out, newline=False):
        """打印到控制台，如果需要，则添加一个换行符

        对于详细的日志记录器，有时希望同时覆盖一行和一行是静态的。
        该方法添加了在行尾打印换行符的功能。

        这对于非状态行（running: <name> 和 <status>: <name>）很有用，
        因为这些行不应该被覆盖，但是运行状态行应该。

        """
        super(VerboseLog, self)._print(grouptools.format(out))
        if newline:
            sys.stdout.write('\n')
            sys.stdout.flush()

    def start(self, name):
        """打印即将运行的测试

        在测试运行开始前，打印一条 running: <testname> 消息。

        """
        super(VerboseLog, self).start(name)
        with self._LOCK:
            self._print('running: {}'.format(name), newline=True)
            self.__name = name
            self._print_summary()

    def _log(self, value):
        """在测试完成后打印一条消息

        该方法打印 <status>: <name>。在调用 super() 之前，它还执行一些魔术
        操作以打印状态行。

        """
        self._print('{0}: {1}'.format(value, self.__name), newline=True)

        # 将 lastlength 设置为 0，这样可以防止在 super() 中打印不必要的填充
        self._state['lastlength'] = 0
        super(VerboseLog, self)._log(value)
```


## DummyLog

```python
class VerboseLog(QuietLog):
    """一个更详细的 QuietLog 版本

    该日志记录器的工作方式类似于安静的日志记录器，只是它永远不会覆盖上一行。
    它还在每个测试开始时打印一行额外的行，这是 quite 日志记录器不执行的操作。

    """
    def __init__(self, *args, **kwargs):
        super(VerboseLog, self).__init__(*args, **kwargs)
        self.__name = None    # 名称需要由 start 和 log 打印

    def _print(self, out, newline=False):
        """打印到控制台，如果需要，则添加一个换行符

        对于详细的日志记录器，有时希望同时覆盖一行和一行是静态的。
        该方法添加了在行尾打印换行符的功能。

        这对于非状态行（running: <name> 和 <status>: <name>）很有用，
        因为这些行不应该被覆盖，但是运行状态行应该。

        """
        super(VerboseLog, self)._print(grouptools.format(out))
        if newline:
            sys.stdout.write('\n')
            sys.stdout.flush()

    def start(self, name):
        """打印即将运行的测试

        在测试运行开始前，打印一条 running: <testname> 消息。

        """
        super(VerboseLog, self).start(name)
        with self._LOCK:
            self._print('running: {}'.format(name), newline=True)
            self.__name = name
            self._print_summary()

    def _log(self, value):
        """在测试完成后打印一条消息

        该方法打印 <status>: <name>。在调用 super() 之前，它还执行一些魔术操作以打印状态行。

        """
        self._print('{0}: {1}'.format(value, self.__name), newline=True)

        # 将 lastlength 设置为 0，这样可以防止在 super() 中打印不必要的填充
        self._state['lastlength'] = 0
        super(VerboseLog, self)._log(value)
```

## HTTPLog

这是一个 Python 类的注释和实现的翻译。这个类是 `QuietLog` 类的子类，提供了一种更详细的日志记录方式。以下是翻译的注释和实现：

```python
class VerboseLog(QuietLog):
    """一个更详细的 QuietLog 版本

    该日志记录器的工作方式类似于安静的日志记录器，只是它永远不会覆盖上一行。
    它还在每个测试开始时打印一行额外的行，这是 quite 日志记录器不执行的操作。

    """
    def __init__(self, *args, **kwargs):
        super(VerboseLog, self).__init__(*args, **kwargs)
        self.__name = None    # 名称需要由 start 和 log 打印

    def _print(self, out, newline=False):
        """打印到控制台，如果需要，则添加一个换行符

        对于详细的日志记录器，有时希望同时覆盖一行和一行是静态的。
        该方法添加了在行尾打印换行符的功能。

        这对于非状态行（running: <name> 和 <status>: <name>）很有用，
        因为这些行不应该被覆盖，但是运行状态行应该。

        """
        super(VerboseLog, self)._print(grouptools.format(out))
        if newline:
            sys.stdout.write('\n')
            sys.stdout.flush()

    def start(self, name):
        """打印即将运行的测试

        在测试运行开始前，打印一条 running: <testname> 消息。

        """
        super(VerboseLog, self).start(name)
        with self._LOCK:
            self._print('running: {}'.format(name), newline=True)
            self.__name = name
            self._print_summary()

    def _log(self, value):
        """在测试完成后打印一条消息

        该方法打印 <status>: <name>。在调用 super() 之前，它还执行一些魔术
        操作以打印状态行。

        """
        self._print('{0}: {1}'.format(value, self.__name), newline=True)

        # 将 lastlength 设置为 0，这样可以防止在 super() 中打印不必要的填充
        self._state['lastlength'] = 0
        super(VerboseLog, self)._log(value)
```


# framework.profile


```py
def load_test_profile(filename, python=None):
    """加载一个 Python 模块并返回其 'profile' 属性。

    所有的 Python 测试文件都提供一个 'profile' 属性，它是一个 TestProfile 实例。这个函数加载该模块并返回它，或者引发一个错误。

    该方法不关心文件扩展名，以保持与用于包装 Piglit 的脚本的兼容性。'tests/quick'、'tests/quick.tests'、'tests/quick.py' 和 'quick' 在文件名上都是等效的。

    如果模块不存在，或者模块没有 'profile' 属性，这将引发一个 FatalError。

    Raises:
    PiglitFatalError -- 如果由于任何原因无法导入模块，或者模块缺少 "profile" 属性。

    Arguments:
    filename -- 要从中获取 'profile' 的 Python 模块的名称

    Keyword Arguments:
    python -- 如果为 None（默认值），则尝试 XML，然后尝试 Python 模块。如果为 True，则只尝试 Python，如果为 False，则只尝试 XML。
    """
    name, ext = os.path.splitext(os.path.basename(filename))
    if ext == '.no_isolation':
        name = filename

    if not python:
        # 如果进程隔离为 false，则尝试加载名为 {name}.no_isolation 的 profile。这仅对基于 XML 的 profile 有效。
        if ext != '.no_isolation' and not OPTIONS.process_isolation:
            try:
                return load_test_profile(name + '.no_isolation' + ext, python)
            except exceptions.PiglitFatalError:
                # 可能没有 .no_isolation 版本，尝试加载常规版本
                pass

        if os.path.isabs(filename):
            if '.meta' in filename:
                return MetaProfile(filename)
            if '.xml' in filename:
                return XMLProfile(filename)

        meta = os.path.join(ROOT_DIR, 'tests', name + '.meta.xml')
        if os.path.exists(meta):
            return MetaProfile(meta)

        xml = os.path.join(ROOT_DIR, 'tests', name + '.xml.gz')
        if os.path.exists(xml):
            return XMLProfile(xml)

    if python is False:
        raise exceptions.PiglitFatalError(
            '无法打开 "tests/{0}.xml.gz" 或 "tests/{0}.meta.xml"'.format(name))

    try:
        mod = importlib.import_module('tests.{0}'.format(name))
    except ImportError:
        raise exceptions.PiglitFatalError(
            '导入 "{}" 失败，模块可能存在问题或不存在。检查拼写是否正确？'.format(filename))

    try:
        return mod.profile
    except AttributeError:
        raise exceptions.PiglitFatalError(
            '在模块 {} 中没有 "profile" 属性。\n'
            '您是否指定了正确的文件？'.format(filename))

```

# PIGLIT 测试 CTS 

# 环境变量

## PIGLIT_PLATFORM
## PIGLIT_BUILD_DIR

ROOT_DIR = PIGLIT_BUILD_DIR

TEET_BIN_DIR



