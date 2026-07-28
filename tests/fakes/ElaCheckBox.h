#ifndef TESTS_FAKES_ELACHECKBOX_H
#define TESTS_FAKES_ELACHECKBOX_H

#include <QCheckBox>

// 布局测试只需要 QCheckBox 的尺寸行为，避免引入完整第三方界面库。
class ElaCheckBox : public QCheckBox
{
  public:
    using QCheckBox::QCheckBox;
};

#endif // TESTS_FAKES_ELACHECKBOX_H
