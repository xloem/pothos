// Copyright (c) 2014-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include "PlotUtilsConfig.hpp"
#include <qwt_plot_zoomer.h>

class QwtRasterData;

/*!
 * Plot picker with custom marker style
 */
class POTHOS_PLOTTER_UTILS_EXPORT MyPlotPicker : public QwtPlotZoomer
{
    Q_OBJECT
public:
    MyPlotPicker(QWidget *parent);

    //! support for getting a z-axis value
    void registerRaster(QwtRasterData *raster);

protected:
    QwtText trackerTextF(const QPointF &pos) const;
    void widgetMouseDoubleClickEvent(QMouseEvent *);
    bool accept(QPolygon &pa) const;

private:
    QwtRasterData *_raster;
};
