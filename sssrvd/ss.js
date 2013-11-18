
function getModelsForCurrentTask() {
    var task = $("#tasklist").val();
	if (task) {
		getModels(task);
	}
}

function getModels(taskNm) {
	$.getJSON(
		"getmodels?task=" + taskNm,
		function(data) {
			var modellist = $("#modellist");
			modellist.html("");
			for (var i = 0; i < data.length; i++) {
				var model = data[i];
				modellist.append("<option value=\"" + model.model + "\">" + model.model +"</option>");  
			}			
		}
	).error(function(jqxhr, status, error) { alert(jqxhr.responseText + " " + status + " " + error); })
}

function getTasks() {
    $.getJSON(
		"gettasks",
		function (data) {
		    // populate tasks for training
		    var tasklist = $("#tasklist");
		    tasklist.html("");
		    for (var i = 0; i < data.length; i++) {
		        var task = data[i];
		        tasklist.append("<option value=\"" + task.task + "\">" + task.task + "</option>");
		    }
		    if (!$("#modellist").val()) {
		        getModels(data[0].task);
		    }
		    // populate filters
		    var filters = $("#filters");
		    filters.html("");
		    for (var i = 0; i < data.length; i++) {
		        var task = data[i];

		        var p = $(document.createElement("div"));
		        p.addClass("row");
		        var span = $(document.createElement("div"));
		        span.text(task.task + ": ");
		        span.addClass("span2");
		        p.append(span);
		        var selectSpan = $(document.createElement("div"));
		        selectSpan.addClass("span4");
		        p.append(selectSpan);
		        var select = $(document.createElement("select"));
		        select.attr("name", task.task)
		        select.addClass("filter");
		        select.attr("id", "filter-" + task.task);

		        var classes = task.classes;
		        var optDefault = $(document.createElement("option"));
		        select.append(optDefault);
		        for (var j = 0; j < classes.length; j++) {
		            var opt = $(document.createElement("option"));
		            opt.attr("value", classes[j]);
		            opt.text(classes[j]);
		            select.append(opt);
		        }

		        selectSpan.append(select);
		        filters.append(p);
		    }
		}
	).error(function (jqxhr, status, error) { alert(jqxhr.responseText + " " + status + " " + error); })
}

function createModel(type, name, task) {
    $.ajax({
        url: "addmodel?type=" + type + "&name=" + name + "&task=" + task, 
        success: function (data) {
            getModelsForCurrentTask();
        }
    });
}
function updateModel(name, task) {
	$.ajax("updatemodel?name=" + name + "&task=" + task);
}
function addTask(taskName, classes) {
	$.ajax({
        url:"addtask?task=" + taskName  + "&classes=" + classes, 
        success: function(data) {
            getTasks();
        }
    });
}
function addLabel(task, label, docid, model) {
	$.ajax("addlabel?task=" + task + "&label=" + label + "&docid=" + docid + "&model=" + model);
}

function search(q, n) {
    $.getJSON(
		"search?query=" + q + "&n=" + n,
		function (data) {
		    var positiveList = $("#resultlist-pos");
		    var neutralList = $("#resultlist-neu");
		    var negativeList = $("#resultlist-neg");
		    positiveList.html("");
		    negativeList.html("");
		    neutralList.html("");
		    for (var i = 0; i < data.length; i++) {
		        var element = data[i];
		        var str = getTweetHtml(element, ["positive","negative","neutral"]);
			    neutralList.append(str);
		    }
		    populateRatings();
		}
	).error(function (jqxhr, status, error) { alert(jqxhr.responseText + " " + status + " " + error); })
}

function testCurrentModel() {
    var model = $("#modellist").val();
	if (model) {
		testModel(model);
	}
}
function testModel(model) {
    $.getJSON(
        "testmodel?model=" + model,
        function (data) {
            var testResults = $("#testresults");
            testResults.html("");
            for (var i = 0; i < data.length; i++) {
                var result = data[i];
                testResults.append("Measurement: " + result.class + "<br />");
                testResults.append("Precision: " + result.precision + "<br />");
                testResults.append("Recall: " + result.recall + "<br />");
                testResults.append("F<sub>1</sub>: " + result.f1 +"<br />");
            }

        }
    ).error(function (jqxhr, status, error) { alert(jqxhr.responseText + " " + status + " " + error); })
}

function getTweetHtml(element, classes) {
    var items = [];
    var li = $(document.createElement("li"));
    li.attr("id", element.docid);
    li.attr("title", "Score: " + element.score);
    li.text(element.content);
    for (var i = 0; i < classes.length; i++) {
        var btn = $(document.createElement("input"));
        btn.attr("type", "button");
        btn.attr("name", element.docid);
        btn.addClass("vote");
        btn.addClass("btn");
        btn.addClass("cls" + (i % 5));
        btn.val(classes[i]);
        li.append(btn);
    }
    var btn = $(document.createElement("input"));
    btn.attr("type", "button");
    btn.attr("name", element.docid);
    btn.addClass("vote");
    btn.addClass("btn");
    btn.addClass("nocls");
    btn.val("none");
    li.append(btn);

    return li;
}

function populateUncertain(data) {
    var uncertainList = $("#uncertainlist");
    var classes = data.classes;
    var results = data.results;

    uncertainList.html("");

    for (var i = 0; i < results.length; i++) {
        var element = results[i];
        var str = getTweetHtml(element, classes);
        uncertainList.append(str);
    }

    populateRatings();

}

function toQuery(map) {
    var buf = [];
    for (var key in map) {
        buf.push("&");
        buf.push(key);
        buf.push("=");
        buf.push(map[key]);
    }
    return buf.join("");
}

function searchAndGetUncertain(q, model, n, filters) {
    $.getJSON(
		"searchandgetuncertain?model=" + model + "&n=" + n + "&query=" + q + toQuery(filters),
		populateUncertain
	).error(function (jqxhr, status, error) { alert(jqxhr.responseText + " " + status + " " + error); })
}


function getUncertain(model, n, filters) {
	$.getJSON(
		"getuncertain?model=" + model + "&n=" + n + toQuery(filters),
		populateUncertain	 
	).error(function(jqxhr, status, error) { alert(jqxhr.responseText + " " + status + " " + error); })
}

function searchAndLabel(q, model, n, filters) {
    $.getJSON(
		"searchandlabel?query=" + q + "&model=" + model + "&n=" + n + toQuery(filters),
		function (data) {
		    var classes = data.classes;
		    var results = data.results;

		    var classToStyle = {};
		    for (var i = 0; i < classes.length; i++) {
		        classToStyle[classes[i]] = "cls" + (i % 5);
		    }
		    classToStyle[""] = "nocls";

		    var lists = {};
		    for (var i = 0; i < classes.length; i++) {
		        var lst = $(document.createElement("ul"));
		        lst.attr("id", "resultlist-" + classes[i]);
		        lst.addClass(classToStyle[classes[i]]);
		        lists[classes[i]] = lst;
		    }
		    var lst = $(document.createElement("ul"));
		    lst.attr("id", "resultlist-none");
		    lst.addClass("nocls");
		    lists[""] = lst;


		    for (var i = 0; i < results.length; i++) {
		        var element = results[i];
		        var str = getTweetHtml(element, classes);
		        var lst = lists[element.label];
		        lst.append(str);
		    }
		    var resultDiv = $("#results");
		    resultDiv.html("");
		    for (var i = 0; i < classes.length; i++) {
		        resultDiv.append("<strong>" + classes[i] + ":</strong>");
		        var lst = lists[classes[i]];
		        if (lst.children().length == 0) {
		            resultDiv.append($("<p><small>No items classified into this class</small></p>"));
		        } else {
		            resultDiv.append(lst);
		        }
		    }
		    resultDiv.append("<strong>unclassified:</strong>");
		    resultDiv.append(lists[""]);

		    populateRatings();
		}
	).error(function (jqxhr, status, error) { alert(jqxhr.responseText + " " + status + " " + error); })
}

function hello() {
	$.ajax("hello", function(data) {
		alert(data)
	}).success(function() { alert("second success"); })
	.error(function(jqxhr, status, error) { alert("aaerror" + jqxhr.responseText + " " + status + " " + error); })
	.complete(function() { alert("complete"); });
}

function populateRatings() {
    $("input.vote").each(function (i, e) {
        var elt = $(e);
        var docid = elt.attr("name");
        var value = elt.val();
        elt.click(function () {

            var taskNm = $("#tasklist").val();
            var model = $("#modellist").val();
            addLabel(taskNm, value, docid, model);
        });
    });
	
}

$(document).ready(function () {
    $("input#search").click(function (e) {
        e.preventDefault();
        var query = $("input#query").val()
        var model = $("#modellist").val();
        var filterSelects = $("select.filter");
        var filters = {};
        for (var i = 0; i < filterSelects.length; i++) {
            var filterSelect = $(filterSelects[i]);
            var key = filterSelect.attr("name");
            var value = filterSelect.val();
            if (value != "" && value != null) {
                filters[key] = value;
            }
        }
        if (model) {
            if (query) {
                searchAndLabel(query, model, 15, filters);
                searchAndGetUncertain(query, model, 5, filters);
            } else {
                getUncertain(model, 5, filters);
            }
        }
    });


    $("input#createmodel").click(function () {
        var model = $("input#model").val();
        var taskNm = $("#tasklist").val();
        createModel("NBMulti", model, taskNm);
    });

    $("input#updatemodel").click(function () {
        var model = $("#modellist").val();
        var taskNm = $("#tasklist").val();
        updateModel(model, taskNm);
    });

    $("input#addtask").click(function () {
        var taskNm = $("input#task").val();
        var taskClasses = $("input#classes").val();
        addTask(taskNm, taskClasses);
    });

    getTasks();

    $("#tasklist").change(getModelsForCurrentTask);


    $("input#hello").click(hello);
    $("input#testmodel").click(testCurrentModel);

    getModelsForCurrentTask();
});
