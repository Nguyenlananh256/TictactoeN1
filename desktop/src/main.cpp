#include "window.h"
#include <QApplication>
#include <QTimer>
#include <QPixmap>
#include <QFontDatabase>
#include "mcts.h"
#include "random.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Caro Arena");
    app.setOrganizationName("TictactoeN1");
    app.setStyle("Fusion");
    QFontDatabase::addApplicationFont(":/fonts/PatrickHand-Regular.ttf");
    CaroWindow window;
    // Register the team's algorithms here. See README.md for the C++ contract.
    window.registerAI("mcts", chooseMcts);
    window.registerAI("random", chooseRandom);

    window.show();
    // Developer-only rendering hook for visual verification without UI automation.
    const auto arguments = app.arguments();
    const int preview = arguments.indexOf("--render-preview");
    if (preview >= 0 && preview + 1 < arguments.size()) {
        QTimer::singleShot(250, &window, [&app, &window, path = arguments[preview + 1]] {
            app.exit(window.grab().save(path) ? 0 : 1);
        });
    }
    return app.exec();
}
