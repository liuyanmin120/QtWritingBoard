#include "wbcanvasitem.h"
#include <QDebug>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include "wbcommondef.h"
#include <QPointer>
#include <QTime>

#define PENWIDTH 20

WbCanvasItem::WbCanvasItem(const QSizeF & size,QGraphicsObject * parent):
    QGraphicsObject (parent),
    m_size(size)
{
    initCanvas();
}

WbCanvasItem::~WbCanvasItem()
{
    SAFE_DELETE(m_pRealPainter);
    SAFE_DELETE(m_pRealCanvas);
    qDebug()  << __FUNCTION__ << "after";
}

void WbCanvasItem::drawPress(int id, const QPointF &p)
{
    CLineObj * obj = new CLineObj(p);
//    obj->addToPath(p,p);
    m_lineObjs.insert(id,obj);
//    m_pTempLayer->drawToTemp(obj);
}

void WbCanvasItem::drawMove(int id, const QPointF &lastPoint, const QPointF &curPoint)
{
    CLineObj * obj = m_lineObjs.value(id,nullptr);
    if(!obj) return;
    obj->addToPath(lastPoint,curPoint);

    if(m_curMode == Mode_DrawLine){
#if 0
        if(obj->elementCount() < 300){
            m_pTempLayer->drawToTemp(obj);
        }
        else{
#ifndef DRAW_VECTOR
            drawToReal(obj);       //绘制真实层 非矢量线
#else
            drawToRealByVector(obj);       //绘制真实层，矢量线
#endif
            obj->createNewPath();   //清空画线
        }
#else
#ifndef DRAW_VECTOR
        drawToReal(obj);
        obj->createNewPath();   //清空画线
#else
        m_pTempLayer->drawToTemp(obj);
        obj->createNewPath();   //清空画线
#endif

#endif
    }
    else{
        doErase(lastPoint,curPoint,30);
    }
}

void WbCanvasItem::drawRelease(int id, const QPointF &point)
{
    CLineObj * obj = m_lineObjs.value(id,nullptr);
    if(!obj) return;
    obj->addToPath(point,point);
    if(m_curMode == Mode_DrawLine){
#ifdef DRAW_VECTOR
        drawToRealByVector(obj);  //绘制真实层，矢量线
        obj->createNewRealPath();   //清空画线
#else
//        drawToReal(obj);  //绘制真实层 非矢量线
#endif
    }
    else{
        doErase(point,point,30);
    }

    m_lineObjs.remove(id);
    delete obj;
    obj = nullptr;

    m_nFlags = 0;
    m_nCount = 0;
}

void WbCanvasItem::setBackgroundColor(const QColor &color)
{
    m_bgColor = color;
    m_pRealCanvas->fill(color);
}

void WbCanvasItem::setMode(WbCanvasItem::DrawMode mode)
{
    m_curMode = mode;
}

void WbCanvasItem::clear()
{
#ifdef DRAW_VECTOR
    QList<QGraphicsItem*> items = this->childItems();
    QList<QGraphicsItem*>::iterator it = items.begin();
    while(it != items.end()){
        QGraphicsItem* item = *it++;
        if(item->type() == Type_LineVector){
            delete item;
            item = nullptr;
        }
    }
#else
    m_pRealCanvas->fill(m_bgColor);
#endif
    update();
}

void WbCanvasItem::zoomIn(qreal sc)
{
#ifdef DRAW_VECTOR
    QList<QGraphicsItem*> items = this->childItems();
    QList<QGraphicsItem*>::iterator it = items.begin();
    while(it != items.end()){
        QGraphicsItem* item = *it++;
        if(item->type() == Type_LineVector){
            item->setScale(item->scale() + sc);
        }
    }
#else
    this->setScale(this->scale() + sc);
#endif
}

void WbCanvasItem::zoomOut(qreal sc)
{
#ifdef DRAW_VECTOR
    QList<QGraphicsItem*> items = this->childItems();
    QList<QGraphicsItem*>::iterator it = items.begin();
    while(it != items.end()){
        QGraphicsItem* item = *it++;
        if(item->type() == Type_LineVector){
            item->setScale(item->scale() - sc);
        }
    }
#else
    this->setScale(this->scale() - sc);
#endif
}

QRectF WbCanvasItem::boundingRect() const
{
    return  QRectF(0,0,m_size.width(),m_size.height());
}

void WbCanvasItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
#ifndef DRAW_VECTOR
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawImage(0,0,*m_pRealCanvas);
#endif
}

void WbCanvasItem::resize(const QSizeF &size)
{
    m_size = size;
}


void WbCanvasItem::drawToReal(CLineObj *obj)
{
    m_pRealPainter->setRenderHint(QPainter::Antialiasing, true);
    m_pRealPainter->setCompositionMode(QPainter::CompositionMode_Source);
    m_pRealPainter->setPen(QPen(Qt::red,PENWIDTH, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPainterPath path;

#if 0 //绘制带笔锋的线条
    QPainterPath lastPath = obj->StrokeLastPath(PENWIDTH);
    m_pRealPainter->fillPath(lastPath,Qt::red);//填充轮廓
    path = obj->StrokePath(3);
    m_pRealPainter->fillPath(path,Qt::red);//填充轮廓
    drawPatchPoint(m_pRealPainter,obj->path());

#elif 1 //绘制钢笔

    m_nCount++;

//    if(m_nCount < 4){
//       path = obj->path();
////       qDebug() << "--->>>Lynn<<<---" << __FUNCTION__ << "++++++++++++" << path.length();
//       drawFirstPath(m_pRealPainter,path,m_nCount,PENWIDTH);
//    }
//    //绘制曲线头，第一条path
    /*else */{
        //绘制曲线体
        qreal lastPathWidth = calPathWidth(obj->lastPath());

        QPainterPath lastPath = obj->StrokeLastPath(lastPathWidth);
//        m_pRealPainter->fillPath(lastPath,Qt::red);//填充轮廓
        qreal pathWidth = calPathWidth(obj->path());
        path = obj->StrokePath(pathWidth);
        m_pRealPainter->fillPath(path,m_colors.at(m_nFlags++%3));//填充轮廓
//        qDebug() << "--->>>Lynn<<<---" << __FUNCTION__ << qAbs(pathWidth - lastPathWidth);
//        if(qAbs(pathWidth - lastPathWidth) > 3){
//            if(pathWidth > lastPathWidth){
//                pathWidth = lastPathWidth + 2;
//            }
//            else {
//                pathWidth = lastPathWidth - 2;
//            }
//        }
        if(lastPathWidth != pathWidth){
            drawPatchPoint2(m_pRealPainter,obj->lastPath(),obj->path(),lastPathWidth,pathWidth);
        }
    }

#else
    path = obj->path();
    drawMarkerPath(m_pRealPainter,path);

#endif
    //清空临时层
    m_pTempLayer->clear();
    this->update(path.boundingRect());
}
//在两个path连接处绘制补充点，使其连接处平滑
void WbCanvasItem::drawPatchPoint(QPainter * painter,QPainterPath path)
{
    qreal temp = (PENWIDTH-3)/100.;
    int k = 0;
    for (qreal i = 0;i < 1; i+=0.01) {
        k++;
        painter->setPen(QPen(Qt::red,PENWIDTH-temp*k,
                             Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPoint(path.pointAtPercent(i));
    }
}

void WbCanvasItem::drawPatchPoint2(QPainter *painter, QPainterPath lastPath,
                                   QPainterPath curPath,
                                   qreal lastWidth, qreal curWidth)
{
    qreal tPatchLength = 100.;
    if(lastWidth < curWidth){
        tPatchLength = calPatchLength(curPath.length());

        qreal temp = (curWidth-lastWidth)/tPatchLength;
        int k = 0;

        for (qreal i = 1;i > (100-tPatchLength)/100.; i-=0.01) {
            k++;
            painter->setPen(QPen(Qt::black,curWidth-temp*k, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawPoint(lastPath.pointAtPercent(i));
        }
    }
    else if(lastWidth > curWidth){

        tPatchLength = calPatchLength(curPath.length());

        qreal temp = (lastWidth-curWidth)/tPatchLength;
        int k = 0;

        for (qreal i = 0;i < tPatchLength/100.; i+=0.01) {
            k++;
            painter->setPen(QPen(Qt::black,lastWidth-temp*k, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawPoint(curPath.pointAtPercent(i));
        }
    }
}

void WbCanvasItem::drawFirstPath(QPainter *painter, QPainterPath path,
                                 int num,qreal normalWidth)
{
    qreal temp = normalWidth/3./100.;
    int k = 0;

    for (qreal i = 0;i < 1; i+=0.01) {
        k++;
//        qDebug() << "--->>>Lynn<<<---" << __FUNCTION__<<temp*k + 10/4.*(num-1);
        painter->setPen(QPen(Qt::red,temp*k + normalWidth/3.*(num-1), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        qDebug() << "--->>>Lynn<<<---" << __FUNCTION__
                 <<path.pointAtPercent(i) << i << k << num;
        painter->drawPoint(path.pointAtPercent(i));
    }
}

void WbCanvasItem::drawMarkerPath(QPainter *painter, QPainterPath path)
{

    QPolygonF polygon;
    // 设置渐变色
//       QLinearGradient linear(QPointF(80, 80), QPointF(150, 150));
//       linear.setColorAt(0, Qt::black);
//       linear.setColorAt(1, Qt::red);

//       // 设置显示模式
//       linear.setSpread(QGradient::PadSpread);

//       // 设置画笔颜色、宽度
//       painter.setPen(QPen(QColor(0, 160, 230), 2));
       // 设置画刷填充
//       painter.setBrush(linear);

    QTransform trans;
    trans.rotate(45);
    for (qreal i = 0;i < 1; i+=0.01) {
        painter->setBrush(Qt::red);
//        painter->setPen(QPen(Qt::red,1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPointF pos = path.pointAtPercent(i);
//
        QRectF rect = QRectF(QPointF(pos.x() - 7,pos.y() - 3),QSizeF(14,6));
        painter->setTransform(trans);
        painter->setBrushOrigin(rect.center());
        painter->drawRoundedRect(rect,7,3);
//        painter->restore();
    }
}

qreal WbCanvasItem::calPathWidth(QPainterPath path)
{
    qreal length = path.length();
    qreal width = PENWIDTH;

    qreal t = length/6. - 1;
    width = PENWIDTH - t;
    if(width < 5){ //最小宽度
        width = 5;
    }
  #if 0
   /* if(length < 10){
        width = 10;
    }
    else */if (length < 40) {
        width = 18;
    }
    else if (length < 60) {
        width = 16;
    }
    else if (length < 80) {
        width = 14;
    }
    else if (length < 100) {
        width = 12;
    }
    else if (length < 120) {
        width = 10;
    }
    else if (length < 150) {
        width = 8;
    }
    else if (length < 180) {
        width = 6;
    }
    else if (length < 210) {
        width = 4;
    }
    else{
        width = 2;
    }
    qDebug() << "--->>>Lynn<<<---" << __FUNCTION__
             << "length = " << length
             << "width = " << width;
#endif
    return width;
}

qreal WbCanvasItem::calPatchLength(qreal length)
{
    qreal len = 100;
    len = 100 - length/10.*length/100.;
    if(len < 10){
        len = 10;
    }
    return len;
}

void WbCanvasItem::drawToRealByVector(CLineObj *obj)
{
    qDebug() << "--->>>Lynn<<<---" << __FUNCTION__;
    QPainterPath path = obj->StrokeRealPath(5);
    QPointer<WbLineVector> item = new WbLineVector(this);
    item->setPath(path);
    //清空临时层
    m_pTempLayer->clear();
}

void WbCanvasItem::initCanvas()
{
    m_pRealCanvas = new QImage(m_size.toSize(),QImage::Format_ARGB32_Premultiplied);
    m_pRealCanvas->fill(Qt::white);
    m_pRealPainter = new QPainter(m_pRealCanvas);

    //临时绘画层
    m_pTempLayer = new WbTempCanvasLayer(m_size,this);
    m_pTempLayer->setZValue(10);

    //m_colors << QColor(Qt::black) << QColor(Qt::black) << QColor(Qt::black);
    m_colors << QColor(Qt::red) << QColor(Qt::blue) << QColor(Qt::cyan);

//    this->setTransformOriginPoint(this->boundingRect().center());

}



void WbCanvasItem::doErase(QPointF pos1, QPointF pos2,int width)
{
    QPainterPath path = createStrokePath(pos1, pos2, width);

#ifdef DRAW_VECTOR  //矢量线擦除
    scene()->setSelectionArea(path);

    QList<QGraphicsItem*> selItems = scene()->selectedItems();
    qDebug()<<__FUNCTION__<<selItems.size();
    QList<QGraphicsItem*>::iterator it = selItems.begin();
    while(it != selItems.end()){
        QGraphicsItem* item = *it++;

        if(item->type() == Type_LineVector){
            WbLineVector *lineroamer = dynamic_cast<WbLineVector *>(item);
            eraseVectorLineRoamer(lineroamer, pos1, pos2, width);
        }
    }

#else //非矢量线擦除
    m_pRealPainter->setRenderHint(QPainter::Antialiasing, true);
    m_pRealPainter->setCompositionMode(QPainter::CompositionMode_Source);
    m_pRealPainter->setPen(QPen(Qt::white,width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    m_pRealPainter->fillPath(path,Qt::white);//填充轮廓
    update();
#endif

}

void WbCanvasItem::eraseVectorLineRoamer(WbLineVector *l, const QPointF &p1, const QPointF &p2, int width)
{
    if(!l->vectorPath().isEmpty()){
        l->doEraseLine(p1, p2, width);
        if(l->vectorPath().isEmpty()){
            l->deleteLater();
        }
    }else{
        //路径为空时，需要把lineroamer删除
        l->deleteLater();
    }
}

QPainterPath WbCanvasItem::createStrokePath(const QPointF &p1, const QPointF &p2, int width)
{
    QPainterPath path;
    path.moveTo(p1);
    path.lineTo(p2);
    if(path.isEmpty()){
        path.addRegion(QRegion(QRect(p1.x()-0.5, p1.y()-0.5, 1, 1),
                                    QRegion::Ellipse));
    }
    QPainterPathStroker stroker;
    stroker.setWidth(width);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    stroker.createStroke(path);
    return stroker.createStroke(path);
}
