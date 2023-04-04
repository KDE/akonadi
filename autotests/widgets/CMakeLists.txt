macro(add_akonadi_isolated_widget_test source)
    add_akonadi_isolated_test(SOURCE ${source} LINK_LIBRARIES KPim6::AkonadiWidgets KF6::Crash)
endmacro()

add_akonadi_isolated_widget_test(tageditwidgettest.cpp)
add_akonadi_isolated_widget_test(tagwidgettest.cpp)
add_akonadi_isolated_widget_test(subscriptiondialogtest.cpp)
add_akonadi_isolated_widget_test(tagselectioncomboboxtest.cpp)
