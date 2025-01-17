#include "Simulator.h"

void BoxSimulator::buildUI() {
  QChartView *personView = new QChartView(this);
  {
    healthySeries->setName("Healthy");
    infectedSeries->setName("Infected");
    infectedSeries->setColor(Qt::red);
    renderPeople();
    connect(healthySeries, &QScatterSeries::clicked, this, &BoxSimulator::infectPerson);

    personChart->setTitle("Person Box");
    personChart->addSeries(healthySeries);
    personChart->addSeries(infectedSeries);
    personChart->createDefaultAxes();
    personChart->axes(Qt::Horizontal).first()->setRange(0, BOX_WIDTH);
    personChart->axes(Qt::Vertical).first()->setRange(0, BOX_HEIGHT);
    personChart->setAnimationOptions(QChart::NoAnimation);
    personView->setRenderHint(QPainter::Antialiasing);
    personView->setChart(personChart);
    QSizePolicy policy = personView->sizePolicy();
    policy.setHorizontalStretch(2);
    personView->setSizePolicy(policy);
  }

  QChartView *energyView = new QChartView(this);
  {
    numHealthySeries->setName("Healthy");
    numInfectedSeries->setName("Infected");
    numInfectedSeries->setColor(Qt::red);
    energyChart->addSeries(numHealthySeries);
    energyChart->addSeries(numInfectedSeries);
    energyChart->setTitle("SIR development");
    energyChart->createDefaultAxes();

    energyChart->axes(Qt::Horizontal).first()->setRange(0, MEASUREMENTS_IN_ENERGY_PLOT);
    energyChart->axes(Qt::Horizontal).first()->setTitleText(QString("Time").arg(STEPS_PER_MEASUREMENT));
    energyChart->axes(Qt::Vertical).first()->setTitleText("Number of people");

    energyView->setRenderHint(QPainter::Antialiasing);
    energyView->setMaximumHeight(400);
    energyView->setMinimumWidth(400);
    energyView->setChart(energyChart);
  }

  QChartView *velocityHistView = new QChartView(this);
  {
    for (size_t bin = 0; bin < VELOCITY_HISTOGRAM_BINS; bin++)
      *velocityHistSet << 1;
    QStackedBarSeries *series = new QStackedBarSeries();
    series->append(velocityHistSet);
    velocityHistChart->addSeries(series);
    velocityHistChart->addAxis(new QValueAxis(), Qt::AlignBottom);
    series->attachAxis(new QValueAxis()); // this axis is not shown, only used for scaling
    velocityHistChart->axes(Qt::Horizontal).first()->setRange(0, VELOCITY_HISTOGRAM_BINS);
    QValueAxis *axisY = new QValueAxis();
    velocityHistChart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    velocityHistChart->setAnimationOptions(QChart::SeriesAnimations);
    velocityHistView->setRenderHint(QPainter::Antialiasing);
    velocityHistView->setChart(velocityHistChart);
    velocityHistView->setMinimumWidth(450);
  }
  updateHistograms();

  connect(stepBtn, &QPushButton::clicked, [=]() { step(); });
  connect(controlBtn, &QPushButton::clicked, [=]() {
    if (controlBtn->text() == "Start") {
      _timerId = startTimer(10);
      // personView->chart()->setAnimationOptions(QChart::NoAnimation);
      velocityHistChart->setAnimationOptions(QChart::NoAnimation);
      std::cout << "Resetting squared mean velocity measurement" << std::endl;
      _start_step = _step;
      totalMeanVelocity = 0;
      controlBtn->setText("Stop");
    } else {
      killTimer(_timerId);
      double meanVel = totalMeanVelocity / ((_step - _start_step) * TAU);
      std::cout << "Squared Mean velocity result: " << meanVel << std::endl;
      // std::cout << "k_B * T / m = " << meanVel / 2 << std::endl;
      controlBtn->setText("Start");
    }
  });
  connect(liftBtn, &QPushButton::clicked, [=]() {
    for (size_t i = 0; i < people.size(); i++)
      people[i].position[1] += BOX_HEIGHT / 3;
    renderPeople();
    measure();
  });
  connect(slowDownBtn, &QPushButton::clicked, [=]() {
    for (size_t i = 0; i < people.size(); i++) {
      people[i].velocity[0] = pow(abs(people[i].velocity[0]), 0.3);
      people[i].velocity[1] = pow(abs(people[i].velocity[1]), 0.3);
    }
    measure();
  });
  connect(bringDownBtn, &QPushButton::clicked, [=]() {
    for (size_t i = 0; i < people.size(); i++)
      if (people[i].position[1] > BOX_HEIGHT * 0.8)
        people[i].position[1] = pow(abs(people[i].position[1]), 0.6);
    renderPeople();
    measure();
  });
  connect(reinitBtn, &QPushButton::clicked, [=]() {
    initRandomly(10);
    renderPeople();
    measure();
  });
  connect(exportBtn, &QPushButton::clicked, [=]() { exportToCSV(); });

  QComboBox *themeBox = new QComboBox();
  themeBox->addItem("Light");
  themeBox->addItem("Dark");
  themeBox->addItem("Cerulean Blue");
  themeBox->addItem("Brown Sand");
  themeBox->addItem("Icy Blue");
  connect(themeBox, &QComboBox::currentIndexChanged, [=]() {
    std::cout << themeBox->currentIndex();
    switch (themeBox->currentIndex()) {
    case 0:
      setTheme(QChart::ChartThemeLight);
      break;
    case 1:
      setTheme(QChart::ChartThemeDark);
      break;
    case 2:
      setTheme(QChart::ChartThemeBlueCerulean);
      break;
    case 3:
      setTheme(QChart::ChartThemeBrownSand);
      break;
    case 4:
      setTheme(QChart::ChartThemeBlueIcy);
      break;
    default:
      break;
    }
  });

  auto mainWidget = new QWidget(this);
  auto mainLayout = new QGridLayout(mainWidget);
  mainLayout->addWidget(personView, 0, 0);
  auto rightChartLayout = new QVBoxLayout();
  rightChartLayout->addWidget(energyView);
  rightChartLayout->addWidget(new QWidget(this));
  mainLayout->addLayout(rightChartLayout, 0, 2);
  mainLayout->addWidget(statsLabel, 1, 0, 1, 2);
  auto buttonLayout = new QHBoxLayout();
  buttonLayout->addWidget(controlBtn);
  buttonLayout->addWidget(stepBtn);
  buttonLayout->addWidget(liftBtn);
  buttonLayout->addWidget(slowDownBtn);
  buttonLayout->addWidget(bringDownBtn);
  buttonLayout->addWidget(reinitBtn);
  buttonLayout->addWidget(exportBtn);
  buttonLayout->addWidget(themeBox);
  mainLayout->addLayout(buttonLayout, 2, 0, 1, 3);
  setCentralWidget(mainWidget);
  setWindowTitle("Person Box Simulator");

  QShortcut *closeShortcut = new QShortcut(Qt::CTRL | Qt::Key_W, this);
  QObject::connect(closeShortcut, &QShortcut::activated, this, [=]() { close(); });

  _timerId = startTimer(10);
}

void BoxSimulator::renderPeople() {
  healthySeries->clear();
  infectedSeries->clear();
  for (size_t i = 0; i < people.size(); i++)
    if (people[i].state == HEALTHY)
      *healthySeries << QPointF(people[i].position[0], people[i].position[1]);
    else
      *infectedSeries << QPointF(people[i].position[0], people[i].position[1]);
}

void BoxSimulator::updateHistograms() {
  computeVelocityHistogram();
  velocityHistChart->axes(Qt::Horizontal).first()->setRange(velocityHist.min, velocityHist.max);
  velocityHistChart->axes(Qt::Vertical).first()->setRange(0, (double)velocityHist.maxHeight);
  velocityHistSet->remove(0, VELOCITY_HISTOGRAM_BINS);
  for (size_t bin = 0; bin < VELOCITY_HISTOGRAM_BINS; bin++)
    *velocityHistSet << velocityHist.heights[bin];
}

void BoxSimulator::measure() {
  energyChart->axes(Qt::Vertical).first()->setRange(0, (double)people.size());

  double measurement = _step / STEPS_PER_MEASUREMENT;
  *numHealthySeries << QPointF(measurement, healthySeries->count());
  *numInfectedSeries << QPointF(measurement, infectedSeries->count());
  if (measurement > MEASUREMENTS_IN_ENERGY_PLOT)
    energyChart->axes(Qt::Horizontal).first()->setRange((measurement - MEASUREMENTS_IN_ENERGY_PLOT), measurement);
  updateHistograms();

  // statsLabel->setText(QString("t = %1 tu,\t E_kin = %2,\t E_pot = %3")
  //                         .arg(QString::number(_step * TAU * ONE_SECOND, 'E', 3), QString::number(E_kin, 'E', 3),
  //                             QString::number(E_pot, 'E', 3)));
}

void BoxSimulator::step() {
  simulate(STEPS_PER_FRAME);
  renderPeople();
  _step += STEPS_PER_FRAME;

  for (size_t i = 0; i < people.size(); i++) {
    people[i].infectionTimer -= 0.01;
    if (people[i].infectionTimer < 0) {
      people[i].state = HEALTHY;
    }

    // q infects p
    for (auto q : people) {
      if (q.state == HEALTHY || q.infectionTimer > INFECTION_TIMER_INFECTIOUS)
        continue; // only infectious people can infect others
      double distance = std::hypot(people[i].position[0] - q.position[0], people[i].position[1] - q.position[1]);
      if (distance < 1e-5)
        continue;

      double infectionProbability = 0.8 * exp(-6 * distance);
      // std::cout << infectionProbability << std::endl;
      if ((double)rand() / RAND_MAX < infectionProbability) {
        people[i].state = INFECTED;
        people[i].infectionTimer = INFECTION_TIMER_MAX;
      }
    }
  }

  if (_step % STEPS_PER_MEASUREMENT == 0)
    measure();
}

void BoxSimulator::timerEvent(QTimerEvent *event) { step(); }

void BoxSimulator::setTheme(QChart::ChartTheme theme) {
  energyChart->setTheme(theme);
  velocityHistChart->setTheme(theme);
  personChart->setTheme(theme);
}

void BoxSimulator::infectPerson(const QPointF &point) {
  personChart->setAnimationOptions(QChart::NoAnimation);
  std::cout << point.x() << ", " << point.y() << std::endl;
  for (size_t i = 0; i < people.size(); i++) {
    if (abs(people[i].position[0] - point.x()) < 1e-6 && abs(people[i].position[1] - point.y()) < 1e-6) {
      std::cout << "Found: " << people[i].position[0] << ", " << people[i].position[1] << std::endl;
      people[i].state = State::INFECTED;
      people[i].infectionTimer = INFECTION_TIMER_MAX;
      break;
    }
  }
  // QTimer *timer = new QTimer(this);
  // connect(timer, &QTimer::timeout, [=] { renderPeople(); });
  // timer->start(100);
}
