#include "coinpage.h"
#include "ui_coinpage.h"

#include "mainwindow.h"

#include <QJsonParseError>
#include <QJsonArray>

#include <QListView>
#include <QDesktopServices>

#include <QCategoryAxis>

CoinPage::CoinPage(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CoinPage),
    m_tooltip(0)
{
    ui->setupUi(this);

    manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(managerFinished(QNetworkReply*)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), &loop, SLOT(quit()));
    connect(manager, SIGNAL(finished(QNetworkReply*)), &loopNews, SLOT(quit()));
    connect(manager, SIGNAL(finished(QNetworkReply*)), &loopNewsImg, SLOT(quit()));
}

CoinPage::~CoinPage()
{
    delete ui;
}

void CoinPage::resetReq() {
    reqIcon = false;
    reqChart = false;
    searchNews = false;
    reqNews = false;
    reqNewsImage = false;
}

void CoinPage::managerFinished(QNetworkReply* reply) {
    QPixmap pixmap;
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error in" << reply->url() << ":" << reply->errorString();
        return;
    }
    if(reqIcon) {
        QByteArray img = reply->readAll();
        pixmap.loadFromData(img);
        pixmap = pixmap.scaled(QSize(32,32), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->CoinImage->setPixmap(pixmap);

    }
    if(reqChart) {
        m_tooltip = 0;
        QSplineSeries* serie = returnSerie(reply);
        drawChartLine(serie);
    }

    if(searchNews) {
        requestNews(reply);
    }

    if(reqNews) {
        updateNews(reply);
    }

    if(reqNewsImage) {
        updateNewsImage(reply);
    }
    resetReq();
}

void CoinPage::constructor(QNetworkReply *reply, MainWindow* parent) {
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
    if(jsonError.error != QJsonParseError::NoError) {
        qDebug() << "fromJson failed: " << jsonError.errorString();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    mw =parent;

    //header section
    section1(jsonObj);
    section2(jsonObj);
    section2Links(jsonObj);

    //chart
    ui->chartCoinName->setText(jsonObj["name"].toString() + " Price Chart");
    on_changeTimeToday_2_clicked();

    //Coin Desc
    CoinDescSection(jsonObj);

    //Coin News
    searchNewsFunction(jsonObj);
}

void CoinPage::section1(QJsonObject jsonObj) {
    QLocale locale(QLocale::English);
    coinId = jsonObj["id"].toString();
    ui->CoinName->setText(jsonObj["name"].toString());
    ui->CoinSymbol->setText(jsonObj["symbol"].toString().toUpper());
    ui->CoinRank->setText("Rank #" + QString::number(jsonObj["market_cap_rank"].toInteger()));
    if(jsonObj["categories"].toArray().size() == 0) {
        ui->CoinType->setVisible(false);
    }else {
        ui->CoinType->setText(jsonObj["categories"].toArray()[0].toString());
    }

    ui->CoinPriceSmall->setText(ui->CoinName->text() + " Price (" + ui->CoinSymbol->text() + ")");
    double price = jsonObj["market_data"].toObject()["current_price"].toObject()["usd"].toDouble();
    if(price < 1) {
        ui->CoinPrice->setText("$" + locale.toString(price, 'f', 6));
    }else {
        ui->CoinPrice->setText("$" + locale.toString(price, 'f', 2));
    }

    if(jsonObj["market_data"].toObject()["price_change_percentage_24h"].toDouble() >= 0) {
        ui->CoinChange->setStyleSheet("color: white;\nbackground-color: rgb(61, 174, 35);\nborder-radius: 5px;\npadding: 10px 15px;\nmargin-left: 15px;");
        ui->CoinChange->setText("▲" + QString::number(jsonObj["market_data"].toObject()["price_change_percentage_24h"].toDouble(), 'f', 2) + "%");
    }else {
        ui->CoinChange->setStyleSheet("color: white;\nbackground-color: rgb(208, 2, 27);\nborder-radius: 5px;\npadding: 10px 15px;\nmargin-left: 15px;");
        ui->CoinChange->setText("▼" + QString::number(jsonObj["market_data"].toObject()["price_change_percentage_24h"].toDouble(), 'f', 2) + "%");
    }
    ui->CoinLow->setText("Low(24h):$" + locale.toString(jsonObj["market_data"].toObject()["low_24h"].toObject()["usd"].toDouble(), 'f', 2));
    ui->CoinHigh->setText("High(24h):$" + locale.toString(jsonObj["market_data"].toObject()["high_24h"].toObject()["usd"].toDouble(), 'f', 2));

    request.setUrl(QUrl(jsonObj["image"].toObject()["large"].toString()));
    reqIcon = true;
    manager->get(request);
    loop.exec();
}

void CoinPage::section2(QJsonObject jsonObj) {
    QLocale locale(QLocale::English);
    ui->CoinMarketCap->setText("$" + locale.toString(jsonObj["market_data"].toObject()["market_cap"].toObject()["usd"].toDouble(), 'f', 0));

    if(jsonObj["market_data"].toObject()["market_cap_change_percentage_24h"].toDouble() >= 0) {
        ui->CoinMarketCapChange->setStyleSheet("color: rgb(61, 174, 35);");
        ui->CoinMarketCapChange->setText("▲ " + QString::number(jsonObj["market_data"].toObject()["market_cap_change_percentage_24h"].toDouble(), 'f', 2) + "%");
    }else {
        ui->CoinMarketCapChange->setStyleSheet("color: rgb(208, 2, 27);");
        ui->CoinMarketCapChange->setText("▼ " + QString::number(jsonObj["market_data"].toObject()["market_cap_change_percentage_24h"].toDouble(), 'f', 2) + "%");
    }

    ui->CoinFullyDiluted->setText("$" + locale.toString(jsonObj["market_data"].toObject()["fully_diluted_valuation"].toObject()["usd"].toDouble(), 'f', 0));

    ui->CoinVolume->setText("$" + locale.toString(jsonObj["market_data"].toObject()["total_volume"].toObject()["usd"].toDouble(), 'f', 0));
    ui->CoinVolumeDivided->setText(locale.toString(jsonObj["market_data"].toObject()["total_volume"].toObject()["usd"].toDouble() / jsonObj["market_data"].toObject()["market_cap"].toObject()["usd"].toDouble(), 'f', 5));

    ui->CoinSupply->setText(locale.toString(jsonObj["market_data"].toObject()["circulating_supply"].toDouble(), 'f', 0) + " " + jsonObj["symbol"].toString().toUpper());
    ui->CoinSupplyTotal->setText(locale.toString(jsonObj["market_data"].toObject()["total_supply"].toDouble(), 'f', 0));
    if(jsonObj["market_data"].toObject()["max_supply"].isNull()) {
        ui->CoinSupplyMax->setText("--");
    }else {
        ui->CoinSupplyMax->setText(locale.toString(jsonObj["market_data"].toObject()["max_supply"].toDouble(), 'f', 0));
    }
}

void CoinPage::setUpComboBox(QJsonObject jsonObj, QString section, QComboBox* cb) {
    QJsonArray arr = jsonObj["links"].toObject()[section].toArray();
    if(arr.size() == 0 or arr[0].toString() == "") {
        cb->setVisible(false);
    }else {
        for(int i = 0; i < arr.size(); i++) {
            if(arr[i].toString() == "") {
                return;
            }
            cb->addItem(linkShort(arr[i].toString()));
        }
    }
}

QString CoinPage::linkShort(QString link) {
    QUrl url = *new QUrl(link);
    url = url.adjusted(QUrl::RemoveScheme);
    url = url.adjusted(QUrl::RemovePath);
    url = url.adjusted(QUrl::RemovePort);
    return url.toString(QUrl::None).remove("/");
}

void CoinPage::section2Links(QJsonObject jsonObj) {
    QListView* helper;
    helperObj = jsonObj;

    setUpComboBox(jsonObj, "homepage", ui->CoinHomepage);
    helper = qobject_cast<QListView *>(ui->CoinHomepage->view());
    helper->setRowHidden(0, true);
    connect(ui->CoinHomepage, &QComboBox::activated, this, &CoinPage::activateComboBox);

    setUpComboBox(jsonObj, "blockchain_site", ui->CoinWebsite);
    helper = qobject_cast<QListView *>(ui->CoinWebsite->view());
    helper->setRowHidden(0, true);
    connect(ui->CoinWebsite, &QComboBox::activated, this, &CoinPage::activateComboBox);

    setUpComboBox(jsonObj, "official_forum_url", ui->CoinForum);
    helper = qobject_cast<QListView *>(ui->CoinForum->view());
    helper->setRowHidden(0, true);
    connect(ui->CoinForum, &QComboBox::activated, this, &CoinPage::activateComboBox);

    setUpComboBox(jsonObj, "chat_url", ui->CoinChat);
    helper = qobject_cast<QListView *>(ui->CoinChat->view());
    helper->setRowHidden(0, true);
    connect(ui->CoinChat, &QComboBox::activated, this, &CoinPage::activateComboBox);

    setUpComboBox(jsonObj, "announcement_url", ui->CoinAnnouncement);
    helper = qobject_cast<QListView *>(ui->CoinAnnouncement->view());
    helper->setRowHidden(0, true);
    connect(ui->CoinAnnouncement, &QComboBox::activated, this, &CoinPage::activateComboBox);

    //social medias link
    if(jsonObj["links"].toObject()["twitter_screen_name"].toString() != "") {
        ui->CoinSocialMedia->addItem(QIcon(":/CoinPage/icons/CoinPage/twitter.png"), "Twitter");
    }
    if(jsonObj["links"].toObject()["facebook_username"].toString() != "") {
        ui->CoinSocialMedia->addItem(QIcon(":/CoinPage/icons/CoinPage/facebook.png"), "Facebook");
    }
    if(jsonObj["links"].toObject()["telegram_channel_identifier"].toString() != "") {
        ui->CoinSocialMedia->addItem(QIcon(":/CoinPage/icons/CoinPage/telegram.png"), "Telegram");
    }
    if(jsonObj["links"].toObject()["subreddit_url"].toString() != "") {
        ui->CoinSocialMedia->addItem(QIcon(":/CoinPage/icons/CoinPage/reddit.png"), "Reddit");
    }

    helper = qobject_cast<QListView *>(ui->CoinSocialMedia->view());
    helper->setRowHidden(0, true);
    connect(ui->CoinSocialMedia, &QComboBox::activated, this, &CoinPage::activateComboBox);

    //git link
    if(jsonObj["links"].toObject()["github"].toArray().size() == 0 and jsonObj["links"].toObject()["bitbucket"].toArray().size() == 0) {
        ui->CoinRepos->setVisible(false);
    }else {
        if(jsonObj["links"].toObject()["github"].toArray().size() != 0) {
            ui->CoinRepos->addItem(QIcon(":/CoinPage/icons/CoinPage/github.png"), "Github");
        }
        if(jsonObj["links"].toObject()["bitbucket"].toArray().size() != 0) {
            ui->CoinRepos->addItem(QIcon(":/CoinPage/icons/CoinPage/bitbucket.png"), "Bit Bucket");
        }
    }

    helper = qobject_cast<QListView *>(ui->CoinRepos->view());
    helper->setRowHidden(0, true);
    connect(ui->CoinRepos, &QComboBox::activated, this, &CoinPage::activateComboBox);
}

void CoinPage::activateComboBox(int index) {
    QComboBox *s = qobject_cast<QComboBox*> (sender());
    if(s == ui->CoinHomepage) {
        QString url = helperObj["links"].toObject()["homepage"].toArray()[index - 1].toString();
        QDesktopServices::openUrl(QUrl(url));
    }
    if(s == ui->CoinWebsite) {
        QString url = helperObj["links"].toObject()["blockchain_site"].toArray()[index - 1].toString();
        QDesktopServices::openUrl(QUrl(url));
    }
    if(s == ui->CoinForum) {
        QString url = helperObj["links"].toObject()["official_forum_url"].toArray()[index - 1].toString();
        QDesktopServices::openUrl(QUrl(url));
    }
    if(s == ui->CoinChat) {
        QString url = helperObj["links"].toObject()["chat_url"].toArray()[index - 1].toString();
        QDesktopServices::openUrl(QUrl(url));
    }
    if(s == ui->CoinAnnouncement) {
        QString url = helperObj["links"].toObject()["announcement_url"].toArray()[index - 1].toString();
        QDesktopServices::openUrl(QUrl(url));
    }
    if(s == ui->CoinSocialMedia) {
        if(s->itemText(index) == "Twitter") {
            QString url = "https://twitter.com/" + helperObj["links"].toObject()["twitter_screen_name"].toString();
            QDesktopServices::openUrl(QUrl(url));
        }
        if(s->itemText(index) == "Facebook") {
            QString url = "https://www.facebook.com/" + helperObj["links"].toObject()["facebook_username"].toString() + "/";
            QDesktopServices::openUrl(QUrl(url));
        }
        if(s->itemText(index) == "Telegram") {
            QString url = "https://t.me/" + helperObj["links"].toObject()["telegram_channel_identifier"].toString();
            QDesktopServices::openUrl(QUrl(url));
        }
        if(s->itemText(index) == "Reddit") {
            QString url = "https://www.reddit.com/r/" + helperObj["links"].toObject()["subreddit_url"].toString();
            QDesktopServices::openUrl(QUrl(url));
        }
    }

    if(s == ui->CoinRepos) {
        if(s->itemText(index) == "Github") {
            QString url = "https://github.com/" + helperObj["links"].toObject()["repos_url"].toObject()["github"].toArray()[0].toString();
            QDesktopServices::openUrl(QUrl(url));
        }
        if(s->itemText(index) == "Bit Bucket") {
            QString url = "https://bitbucket.org/" + helperObj["links"].toObject()["repos_url"].toObject()["bitbucket"].toArray()[0].toString();
            QDesktopServices::openUrl(QUrl(url));
        }
    }
    s->setCurrentIndex(0);
}

QSplineSeries* CoinPage::returnSerie(QNetworkReply *reply) {
    QJsonParseError jsonError;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);

    QSplineSeries *series = new QSplineSeries();

    if(jsonError.error != QJsonParseError::NoError) {
        qDebug() << "fromJson failed: " << jsonError.errorString();
        return series;
    }



    if(jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        QJsonArray jsonPrice = jsonObj["prices"].toArray();

        QList<double> values = {};


        for(int i = 0; i < jsonPrice.size(); i++) {
            series->append(jsonPrice[i].toArray()[0].toDouble(),jsonPrice[i].toArray()[1].toDouble());
        }
    }
    return series;
}

void CoinPage::drawChartLine(QSplineSeries* series) {

    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(series);

    //chart line
    QPen pen;
    if(series->at(0).y() <= series->at(series->count()-1).y()) {
        pen.setColor(QColor(61, 174, 35));
    }else {
        pen.setColor(QColor(208, 2, 27));
    }
    pen.setWidth(2);
    series->setPen(pen);

    //2 Axis
    QCategoryAxis *axisX = new QCategoryAxis();
    QCategoryAxis *axisY = new QCategoryAxis();

    QPen axisPen(QColor(237,237,237));
    axisPen.setWidth(2);
    axisX->setLinePen(axisPen);
    axisY->setLinePen(axisPen);

    QFont labelsFont;
    labelsFont.setPixelSize(12);
    labelsFont.setWeight(QFont::Bold);
    axisX->setLabelsFont(labelsFont);
    axisY->setLabelsFont(labelsFont);
    QBrush labelBrush(QColor(119,121,123));
    axisX->setLabelsBrush(labelBrush);
    axisY->setLabelsBrush(labelBrush);


    double minX = series->at(0).x();
    double maxX = series->at(0).x();
    double minY = series->at(0).y();
    double maxY = series->at(0).y();
    for(int i = 1; i < series->count(); i++) {
        if(series->at(i).y() > maxY) {
            maxY = series->at(i).y();
        }
        if(series->at(i).y() < minY) {
            minY = series->at(i).y();
        }
        if(series->at(i).x() > maxX) {
            maxX = series->at(i).x();
        }
        if(series->at(i).x() < minX) {
            minX = series->at(i).x();
        }
    }

    //Axis X range
    double gap = (maxX - minX) / 6;
    for(double i = minX + gap; i < maxX; i+=gap) {
        QDateTime timeStamp;
        timeStamp.setMSecsSinceEpoch(i);
        QString helper="";
        if(timeSpan != "1d") {
            QDate dateStamp = timeStamp.date();
            helper= ( QString::number(dateStamp.day()) + "/" + QString::number(dateStamp.month()) );
        }else {
            QTime timeHelper = timeStamp.time();
            helper = ( QString::number(timeHelper.hour()) + ":" + QString::number(timeHelper.minute()) );
        }
        axisX->append(helper, i);
    }
    axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    //Axis Y range
    double gapY = (maxY - minY) / 4;
    axisY->append("", minY - gapY / 2);
    axisY->append(QString::number(minY), minY);
    axisY->append(QString::number(minY + gapY), (minY + gapY));
    axisY->append(QString::number(minY + gapY * 2), (minY + gapY * 2));
    axisY->append(QString::number(minY + gapY * 3), (minY + gapY * 3));
    axisY->append(QString::number(maxY), maxY);
    axisY->append("", maxY + gapY / 2);

    axisY->setRange(minY - gapY / 2, maxY + gapY / 2);
    axisY->setGridLineVisible(false);
    axisY->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    //Set chart
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignRight);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    connect(series, &QXYSeries::hovered, this, &CoinPage::chartCallOut);

    ui->chartView_2->setChart(chart);
    ui->chartView_2->setRenderHint(QPainter::Antialiasing);
}

void CoinPage::chartCallOut(const QPointF &point, bool state) {

    if(m_tooltip == 0) {
        m_tooltip = new Callout(ui->chartView_2->chart());
    }
    QString price = "";
    if(point.y() < 1) {
        price = QString::number(point.y());
    }else {
        price = QString::number(point.y(), 'f', 2);
    }

    QString time = "";
    QDateTime timeStamp;
    timeStamp.setMSecsSinceEpoch(point.x());
    if(timeSpan == "1d") {
        time = timeStamp.time().toString("hh:mm:ss");
    }else {
        time = timeStamp.toString("ddd d MMMM yyyy");
    }

    if (state) {
        m_tooltip->setText(QString("%1\n\nPrice: $%2 ").arg(time).arg(price));
        m_tooltip->setAnchor(point);
        m_tooltip->setZValue(11);
        m_tooltip->updateGeometry();
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}

int CoinPage::timeSpanDateCount(QString timeSpan) {
    QDateTime now = QDateTime::currentDateTimeUtc();
    int monthToAdd = 0;
    if(timeSpan == "1d" or timeSpan == "1w") {
        return -1;
    }else if(timeSpan=="ytd") {
        int currentYear = now.date().year();
        QDate ytd{currentYear, 1, 1};
        return -1 * now.daysTo(ytd.startOfDay());

    }else if(timeSpan=="12m") {
        monthToAdd = 12;
    }
    else {
        monthToAdd = timeSpan.at(0).digitValue();
    }
    QDateTime span = now.addMonths(-1 * monthToAdd);
    return span.daysTo(now);
}

void CoinPage::on_changeTimeToday_2_clicked()
{
    if(timeSpan == "1d") {
        return;
    }
    timeSpan = "1d";
    QString chartUrl = "https://api.coingecko.com/api/v3/coins/" + coinId + "/market_chart?vs_currency=usd&days=1";
    reqChart = true;
    request.setUrl(chartUrl);
    manager->get(request);
    loop.exec();
}


void CoinPage::on_changeTime1W_2_clicked()
{
    if(timeSpan == "1w") {
        return;
    }
    timeSpan = "1w";
    QString chartUrl = "https://api.coingecko.com/api/v3/coins/" + coinId + "/market_chart?vs_currency=usd&days=7";
    reqChart = true;
    request.setUrl(chartUrl);
    manager->get(request);
    loop.exec();
}


void CoinPage::on_changeTime1M_2_clicked()
{
    if(timeSpan == "1m") {
        return;
    }
    timeSpan = "1m";
    int days = timeSpanDateCount(timeSpan);
    QString chartUrl = "https://api.coingecko.com/api/v3/coins/" + coinId + "/market_chart?vs_currency=usd&days=" + QString::number(days);
    reqChart = true;
    request.setUrl(chartUrl);
    manager->get(request);
    loop.exec();
}


void CoinPage::on_changeTime3M_2_clicked()
{
    if(timeSpan == "3m") {
        return;
    }
    timeSpan = "3m";
    int days = timeSpanDateCount(timeSpan);
    QString chartUrl = "https://api.coingecko.com/api/v3/coins/" + coinId + "/market_chart?vs_currency=usd&days=" + QString::number(days);
    reqChart = true;
    request.setUrl(chartUrl);
    manager->get(request);
    loop.exec();
}


void CoinPage::on_changeTime6M_2_clicked()
{
    if(timeSpan == "6m") {
        return;
    }
    timeSpan = "6m";
    int days = timeSpanDateCount(timeSpan);
    QString chartUrl = "https://api.coingecko.com/api/v3/coins/" + coinId + "/market_chart?vs_currency=usd&days=" + QString::number(days);
    reqChart = true;
    request.setUrl(chartUrl);
    manager->get(request);
    loop.exec();
}


void CoinPage::on_changeTime12M_2_clicked()
{
    if(timeSpan == "12m") {
        return;
    }
    timeSpan = "12m";
    int days = timeSpanDateCount(timeSpan);
    QString chartUrl = "https://api.coingecko.com/api/v3/coins/" + coinId + "/market_chart?vs_currency=usd&days=" + QString::number(days);
    reqChart = true;
    request.setUrl(chartUrl);
    manager->get(request);
    loop.exec();
}


void CoinPage::on_changeTimeYTD_2_clicked()
{    if(timeSpan == "ytd") {
        return;
    }
    timeSpan = "ytd";
     int days = timeSpanDateCount(timeSpan);
      QString chartUrl = "https://api.coingecko.com/api/v3/coins/" + coinId + "/market_chart?vs_currency=usd&days=" + QString::number(days);
       reqChart = true;
        request.setUrl(chartUrl);
         manager->get(request);
          loop.exec();
}

void CoinPage::CoinDescSection(QJsonObject jsonObj) {
    ui->CoinWhatIs->setText("What is " + jsonObj["name"].toString() + "(" + jsonObj["id"].toString().toUpper() + ")?");
    ui->CoinDesc->setText(jsonObj["description"].toObject()["en"].toString());
}

void CoinPage::searchNewsFunction(QJsonObject jsonObj) {
    ui->newsWidget1->setVisible(false);
    ui->newsWidget2->setVisible(false);
    ui->newsWidget3->setVisible(false);
    ui->CoinNewsName->setText(jsonObj["name"].toString() + " Latest News");
    QString name = jsonObj["name"].toString().replace(" ", "%20");
    QString link = "https://api.marketaux.com/v1/entity/search?search=" + name + "&api_token=Bu2E0S5O8US05Fz7X6qp0G4VOoKOQlcylzlkj0FN";
    request.setUrl(QUrl(link));
    resetReq();
    searchNews = true;
    manager->get(request);
    loop.exec();
}

void CoinPage::requestNews(QNetworkReply *reply) {
    QJsonParseError jsonError;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);

    if(jsonError.error != QJsonParseError::NoError) {
        qDebug() << "fromJson failed: " << jsonError.errorString();
        return ;
    }

    if(jsonDoc["meta"].toObject()["returned"].toInt() == 0) {
        return;
    }else {
        QJsonArray arr = jsonDoc["data"].toArray();
        for(int i = 0; i < arr.size(); i++) {
            if(arr.at(i).toObject()["type"].toString() == "cryptocurrency" && arr.at(i).toObject()["name"].toString().toLower() == ui->CoinName->text().toLower()) {
                ui->label_7->setVisible(false);
                QString link = "https://api.marketaux.com/v1/news/all?symbols=" + arr.at(i).toObject()["symbol"].toString() + "&language=en&limit=3&api_token=Bu2E0S5O8US05Fz7X6qp0G4VOoKOQlcylzlkj0FN";
                request.setUrl(QUrl(link));
                resetReq();
                reqNews = true;
                ui->newsWidget1->setVisible(true);
                ui->newsWidget2->setVisible(true);
                ui->newsWidget3->setVisible(true);
                manager->get(request);
                loopNews.exec();
                return;
            }
        }
        return;
    }
}

QString newsDescShort(QString input) {
    if(input.length() > 160) {
        input = input.sliced(0, 157);
        input += "...";
    }
    return input;
}

QString newsHeadingShort(QString input) {
    if(input.length() > 80) {
        input = input.sliced(0, 77);
        input += "...";
    }
    return input;
}

void CoinPage::updateNews(QNetworkReply* reply) {
    QJsonParseError jsonError;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);

    if(jsonError.error != QJsonParseError::NoError) {
        qDebug() << "fromJson failed: " << jsonError.errorString();
        return ;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QJsonArray arr = jsonObj["data"].toArray();

    if(arr.size() == 1) {
        ui->newsWidget2->setVisible(false);
        ui->newsWidget3->setVisible(false);

        ui->newsHeading1->setText(newsHeadingShort(arr[0].toObject()["title"].toString()));
        ui->newsDesc1->setText(newsDescShort(arr[0].toObject()["description"].toString()));
        ui->newsSource1->setText(arr[0].toObject()["source"].toString());

        request.setUrl(QUrl(arr[0].toObject()["image_url"].toString()));
        resetReq();
        reqNewsImage = true;
        manager->get(request);
        loopNewsImg.exec();
    }

    else if(arr.size() == 2) {
        ui->newsWidget3->setVisible(false);

        ui->newsHeading1->setText(newsHeadingShort(arr[0].toObject()["title"].toString()));
        ui->newsDesc1->setText(newsDescShort(arr[0].toObject()["description"].toString()));
        ui->newsSource1->setText(arr[0].toObject()["source"].toString());

        request.setUrl(QUrl(arr[0].toObject()["image_url"].toString()));
        resetReq();
        reqNewsImage = true;
        manager->get(request);
        loopNewsImg.exec();

        ui->newsHeading2->setText(newsHeadingShort(arr[1].toObject()["title"].toString()));
        ui->newsDesc2->setText(newsDescShort(arr[1].toObject()["description"].toString()));
        ui->newsSource2->setText(arr[1].toObject()["source"].toString());

        request.setUrl(QUrl(arr[1].toObject()["image_url"].toString()));
        resetReq();
        reqNewsImage = true;
        manager->get(request);
        loopNewsImg.exec();
    }else if(arr.size() == 3) {
        ui->newsHeading1->setText(newsHeadingShort(arr[0].toObject()["title"].toString()));
        ui->newsDesc1->setText(newsDescShort(arr[0].toObject()["description"].toString()));
        ui->newsSource1->setText(arr[0].toObject()["source"].toString());

        request.setUrl(QUrl(arr[0].toObject()["image_url"].toString()));
        resetReq();
        reqNewsImage = true;
        manager->get(request);
        loopNewsImg.exec();

        ui->newsHeading2->setText(newsHeadingShort(arr[1].toObject()["title"].toString()));
        ui->newsDesc2->setText(newsDescShort(arr[1].toObject()["description"].toString()));
        ui->newsSource2->setText(arr[1].toObject()["source"].toString());

        request.setUrl(QUrl(arr[1].toObject()["image_url"].toString()));
        resetReq();
        reqNewsImage = true;
        manager->get(request);
        loopNewsImg.exec();

        ui->newsHeading3->setText(newsHeadingShort(arr[2].toObject()["title"].toString()));
        ui->newsDesc3->setText(newsDescShort(arr[2].toObject()["description"].toString()));
        ui->newsSource3->setText(arr[2].toObject()["source"].toString());

        request.setUrl(QUrl(arr[2].toObject()["image_url"].toString()));
        resetReq();
        reqNewsImage = true;
        manager->get(request);
        loopNewsImg.exec();
    }
}

void CoinPage::updateNewsImage(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error in" << reply->url() << ":" << reply->errorString();
        return;
    }
    QPixmap pixmap;
    QByteArray img = reply->readAll();
    pixmap.loadFromData(img);
    pixmap = pixmap.scaled(410, 192, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if(not img1) {
        img1 = true;
        ui->newsImage1->setPixmap(pixmap);
        return;
    }
    if(not img2) {
        img2 = true;
        ui->newsImage2->setPixmap(pixmap);
        return;
    }
    if(not img3) {
        img3 = true;
        ui->newsImage3->setPixmap(pixmap);
        return;
    }
}

void CoinPage::on_pushButton_clicked()
{
    mw->reconnectMainRowItem();
    mw->show();
    this->close();
    this->setAttribute(Qt::WA_DeleteOnClose);
}

