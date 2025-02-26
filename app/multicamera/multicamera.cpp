/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "multicamera.h"
#include <QApplication>
#include <QMediaService>
#include <QMediaRecorder>
#include <QCameraViewfinder>
#include <QCameraInfo>
#include <QMediaMetaData>

#include <QMessageBox>
#include <QPalette>
#include <QTabWidget>
#include <QtWidgets>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define FONT_SIZE 12
Q_DECLARE_METATYPE(QCameraInfo)

multiCamera::multiCamera()
{
    const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
    int camNum = 0, i = 0, num;
    cameraList = new QList<QCamera*>();
    viewfinderList = new QList<QCameraViewfinder*>();
    hLayoutList = new QList<QHBoxLayout*>();
    QList<QCameraInfo> availableCameras = QCameraInfo::availableCameras();
    if(availableCameras.count() == 1){
        camNum = 9;
    }else {
        camNum = availableCameras.count();
    }

    if(camNum > 0){
        num = qSqrt(camNum);
        if(qreal(num) < qSqrt(camNum)){
            num++;
        }
    }

    for(i=0; i<num; i++){
        QHBoxLayout *layout = new QHBoxLayout;
        hLayoutList->append(layout);
    }

    for (int i=0; i<camNum; i++) {
        QCameraInfo cameraInfo = availableCameras.value(i);
        QCameraViewfinder *viewfinder;
        QCamera *camera;
        QCameraImageCapture *imageCapture;

        viewfinder = new QCameraViewfinder();
        camera = new QCamera(cameraInfo);
        imageCapture = new QCameraImageCapture(camera);
        qDebug() << cameraInfo.description();
        viewfinderList->append(viewfinder);
        cameraList->append(camera);
        camera->setViewfinder(viewfinder);
        QHBoxLayout *layout = hLayoutList->value(i/num);
        if(layout){
            layout->addWidget(viewfinder);
            layout->setMargin(0);
            layout->setSpacing(0);
        }
        QImageEncoderSettings m_imageSettings;
        m_imageSettings.setCodec("jpeg");
        m_imageSettings.setQuality(QMultimedia::VeryHighQuality);
        m_imageSettings.setResolution(QSize(640, 480));
        imageCapture->setEncodingSettings(m_imageSettings);
        camera->unload();
        camera->setCaptureMode(QCamera::CaptureStillImage);
        camera->start();
    }
    QBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    for(i=0; i<hLayoutList->count(); i++){
        QWidget *widget = new QWidget();
        widget->setLayout(hLayoutList->value(i));
        mainLayout->addWidget(widget);
    }
    QPushButton *exitButton = new QPushButton(tr("Exit"));
    connect(exitButton, SIGNAL(clicked(bool)), this, SLOT(on_exitClicked()));
    mainLayout->addWidget(exitButton);
    QWidget *widget = new QWidget;
    widget->setLayout(mainLayout);
    setCentralWidget(widget);
    setWindowState(Qt::WindowMaximized);
    setWindowFlags(Qt::FramelessWindowHint);
}

void multiCamera::closeEvent(QCloseEvent *event)
{
        event->accept();
}

void multiCamera::on_exitClicked()
{
        qApp->exit(0);
}
